
// LazyInstance<Type, Traits>类管理Type的单一实例, 对象在第一次访问时被创建.
// 一般在你需要函数级别的静态对象时, 这个类非常有用, 但需要保证线程安全性.
// Type的构造函数即使在线程冲突的情况下也只会被调用一次. Get()和Pointer()总
// 是返回相同的完全初始化的实例对象. 析构函数在程序退出时调用.
//
// 假设对象被安全的创建, LazyInstance也是线程安全的. 类可用POD初始化, 所以不
// 需要静态的构造函数. 通过base::LinkerInitialized构造一个全局的LazyInstance
// 变量是比较好的做法.
//
// LazyInstance类似单件, 但没有单件的特性. 相同的类型可以有多个LazyInstance,
// 每个LazyInstance维护一个唯一的实例. 它会为Type预分配空间, 避免在堆上创建
// 对象. 这有助于提高实例创建的性能, 减少堆碎片. 需要Type有完整的类型以便确
// 定大小.
//
// 用法示例:
//     static LazyInstance<MyClass> my_instance(base::LINKER_INITIALIZED);
//     void SomeMethod() {
//       my_instance.Get().SomeMethod(); // MyClass::SomeMethod()
//
//       MyClass* ptr = my_instance.Pointer();
//       ptr->DoDoDo(); // MyClass::DoDoDo
//     }

#ifndef __base_lazy_instance_h__
#define __base_lazy_instance_h__

#pragma once

#include <new> // placement new.

#include "atomicops.h"
#include "threading/thread_restrictions.h"

namespace base
{

    template<typename Type>
    struct DefaultLazyInstanceTraits
    {
        static const bool kAllowedToAccessOnNonjoinableThread = false;

        static Type* New(void* instance)
        {
            // 使用placement new在预分配的空间上初始化实例.
            // 圆括号很重要, 强制POD类型初始化.
            return new (instance) Type();
        }
        static void Delete(void* instance)
        {
            // 显式调用析构函数.
            reinterpret_cast<Type*>(instance)->~Type();
        }
    };

    template<typename Type>
    struct LeakyLazyInstanceTraits
    {
        static const bool kAllowedToAccessOnNonjoinableThread = true;

        static Type* New(void* instance)
        {
            return DefaultLazyInstanceTraits<Type>::New(instance);
        }
        // 定义一个空的指针而不是空的删除函数. 可以避开注册到AtExitManager
        // 中, 这样可以在没有AtExitManager的环境中使用LeakyLazyInstanceTraits.
        static void (*Delete)(void* instance);
    };

    template<typename Type>
    void (*LeakyLazyInstanceTraits<Type>::Delete)(void* instance) = NULL;

    // 抽取部分非模板函数, 这样可以把复杂的代码片段放在.cpp文件.
    class LazyInstanceHelper
    {
    protected:
        enum
        {
            STATE_EMPTY    = 0,
            STATE_CREATING = 1,
            STATE_CREATED  = 2
        };

        explicit LazyInstanceHelper(LinkerInitialized x) { /* state_为0 */ }

        // 检查是否需要创建实例. 返回true表示需要创建, 返回false表示其它线程
        // 正在创建, 等待实例创建完成并返回false.
        bool NeedsInstance();

        // 创建实例之后, 注册程序退出是调用的析构函数, 并更新state为
        // STATE_CREATED.
        void CompleteInstance(void* instance, void (*dtor)(void*));

        subtle::Atomic32 state_;

    private:
        DISALLOW_COPY_AND_ASSIGN(LazyInstanceHelper);
    };

    template<typename Type, typename Traits=DefaultLazyInstanceTraits<Type> >
    class LazyInstance : public LazyInstanceHelper
    {
    public:
        explicit LazyInstance(LinkerInitialized x) : LazyInstanceHelper(x) {}

        Type& Get()
        {
            return *Pointer();
        }

        Type* Pointer()
        {
            if(!Traits::kAllowedToAccessOnNonjoinableThread)
            {
                ThreadRestrictions::AssertSingletonAllowed();
            }

            // 如果对象已经被创建, 希望能快速访问.
            if((subtle::NoBarrier_Load(&state_)!=STATE_CREATED) && NeedsInstance())
            {
                // 在|buf_|所在的空间上创建实例.
                instance_ = Traits::New(buf_);
                // LeakyLazyInstanceTraits的Traits::Delete为空.
                void (*dtor)(void*) = Traits::Delete;
                CompleteInstance(this, (dtor==NULL) ? NULL : OnExit);
            }

            return instance_;
        }

        bool operator==(Type* p)
        {
            switch(subtle::NoBarrier_Load(&state_))
            {
            case STATE_EMPTY:
                return p == NULL;
            case STATE_CREATING:
                return static_cast<int8*>(static_cast<void*>(p)) == buf_;
            case STATE_CREATED:
                return p == instance_;
            default:
                return false;
            }
        }

    private:
        // 配合AtExit的适配函数. 应该在单线程中调用, 所以不需要使用原子操作.
        // 当其它线程使用实例时调用OnExit是错误的.
        static void OnExit(void* lazy_instance)
        {
            LazyInstance<Type, Traits>* me =
                reinterpret_cast<LazyInstance<Type, Traits>*>(lazy_instance);
            Traits::Delete(me->instance_);
            me->instance_ = NULL;
            subtle::Release_Store(&me->state_, STATE_EMPTY);
        }

        int8 buf_[sizeof(Type)]; // Type实例的预分配空间.
        Type* instance_;

        DISALLOW_COPY_AND_ASSIGN(LazyInstance);
    };

} //namespace base

#endif //__base_lazy_instance_h__