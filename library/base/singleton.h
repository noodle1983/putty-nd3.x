
#ifndef __base_singleton_h__
#define __base_singleton_h__

#pragma once

#include "at_exit.h"
#include "atomicops.h"
#include "threading/thread_restrictions.h"

namespace base
{

    // Singleton<Type>缺省的traits. 调用new和delete创建销毁对象.
    // kRegisterAtExit注册进程退出时是否自动销毁.
    // 如果需要其它参数或者不同的内存分配方式可以进行重载.
    template<typename Type>
    struct DefaultSingletonTraits
    {
        // 分配对象.
        static Type* New()
        {
            // ()在这里非常重要, 强制POD类型初始化.
            return new Type();
        }

        // 销毁对象.
        static void Delete(Type* x)
        {
            delete x;
        }

        // true: 进程退出时自动删除对象.
        static const bool kRegisterAtExit = true;

        // false: 禁止访问non-joinable线程. 与kRegisterAtExit不同, 因为
        // StaticMemorySingletonTraits允许访问non-joinable线程, 对此做了很好的处理.
        static const bool kAllowedToAccessOnNonjoinableThread = false;
    };


    // Singleton<Type>另一个可选的traits, 指明程序退出时不清除对象.
    template<typename Type>
    struct LeakySingletonTraits : public DefaultSingletonTraits<Type>
    {
        static const bool kRegisterAtExit = false;
        static const bool kAllowedToAccessOnNonjoinableThread = true;
    };


    // Singleton<Type>的另外一种traits. 在一块静态的内存上实例化单件. 单件在
    // 程序退出的时候清理, 只有在调用Resurrect()方法进行销毁后才能重用.
    //
    // 在某些地方是有用的, 尤其是logging和tracing这些单件实例在销毁期还能访问
    // 的情况下.
    // 在logging和tracing中, 可能会在特殊的地方被调用, 比如静态析构期, 线程关闭
    // 时等, 这个时候堆上的单件存在销毁冲突. 如果一个线程调用get(), 此时另一个
    // 线程正在执行AtExit处理, 第一个线程可能会访问未分配空间的对象. 如果实例
    // 在数据段分配的话, 它的生命周期一直存在.
    //
    // 析构函数回收系统资源, 会反注册系统的日志等级修改回调.
    template<typename Type>
    struct StaticMemorySingletonTraits
    {
        static Type* New()
        {
            if(NoBarrier_AtomicExchange(&dead_, 1))
            {
                return NULL;
            }
            Type* ptr = reinterpret_cast<Type*>(buffer_);

            // 由内存屏障进行保护.
            new(ptr) Type();
            return ptr;
        }

        static void Delete(Type* p)
        {
            NoBarrier_Store(&dead_, 1);
            MemoryBarrier();
            if(p != NULL)
            {
                p->Type::~Type();
            }
        }

        static const bool kRegisterAtExit = true;
        static const bool kAllowedToAccessOnNonjoinableThread = true;

        // 用于单元测试.
        static void Resurrect()
        {
            NoBarrier_Store(&dead_, 0);
        }

    private:
        static const size_t kBufferSize = (sizeof(Type) +
            sizeof(intptr_t) - 1) / sizeof(intptr_t);
        static intptr_t buffer_[kBufferSize];

        // 标记对象已经被删除, 不再存在.
        static Atomic32 dead_;
    };

    template<typename Type> intptr_t
        StaticMemorySingletonTraits<Type>::buffer_[kBufferSize];
    template<typename Type> Atomic32
        StaticMemorySingletonTraits<Type>::dead_ = 0;


    // Singleton<Type, Traits>类在第一次使用的时候创建
    // 一个实例, 程序正常退出的时候销毁. 非正常退出不会调用Trait::Delete.
    //
    // Singleton<>没有非静态成员也不需要实例化. 当然实例化为类成员或者全局对象
    // 是没有问题的, 因为它是POD类型的.
    //
    // 这个类是线程安全的, 如果想并发使用那么Type类也必须是线程安全的,
    // 用户根据设计需要保证两部分配合.
    //
    // 术语:
    //   RAE = kRegisterAtExit
    //
    // 如果Traits::RAE是true, 单件在进程退出的时候会销毁, 准确的说调用AtExitManager
    // 需要实例化一个这种类型的对象. AtExitManager模拟了atexit()的LIFO序的语义但是
    // 调用上更安全一些. 更多内容参见at_exit.h.
    //
    // 如果Traits::RAE是false, 单件在进程退出的时候不会释放, 因此会有泄漏(假如已创建).
    // Traits::RAE非必要一般不要设置为false, 因为创建的对象可能会被CRT销毁.
    //
    // 如果你想设计一个单件, 将类构造函数private, 并设置DefaultSingletonTraits<>友元.
    //
    //   #include "singleton.h"
    //   class FooClass {
    //    public:
    //     static FooClass* GetInstance(); <--参见下面的注释.
    //     void Bar() { ... }
    //    private:
    //     FooClass() { ... }
    //     friend struct DefaultSingletonTraits<FooClass>;
    //
    //     DISALLOW_COPY_AND_ASSIGN(FooClass);
    //   };
    //
    // 在源文件中:
    //   FooClass* FooClass::GetInstance() {
    //    return Singleton<FooClass>::get();
    //   }
    //
    // 调用FooClass的方法:
    //   FooClass::GetInstance()->Bar();
    //
    // 注意:
    // (a) 每次调用get(),operator->()和operator*()都有损耗(16ns on my P4/2.8GHz)用于
    //     判断对象是否已经初始化. 可以缓存get()结果, 因为指针不会变化.
    //
    // (b) 对象实例化函数不要抛出异常, 因为类不是异常安全(exception-safe)的.
    template<typename Type, typename Traits=DefaultSingletonTraits<Type>,
        typename DifferentiatingType=Type>
    class Singleton
    {
    public:
        // 使用Singleton<T>的类需要声明一个GetInstance()方法调用Singleton::get().
        friend Type* Type::GetInstance();

        // 拷贝构造和赋值构造是安全的, 因为没有任何成员.

        // 返回Type类的实例指针
        static Type* get()
        {
            if(!Traits::kAllowedToAccessOnNonjoinableThread)
            {
                ThreadRestrictions::AssertSingletonAllowed();
            }

            // AtomicWord也起到自旋锁的作用, kBeingCreatedMarker表示自旋锁等待创建.
            static const AtomicWord kBeingCreatedMarker = 1;

            AtomicWord value = NoBarrier_Load(&instance_);
            if(value!=0 && value!=kBeingCreatedMarker)
            {
                return reinterpret_cast<Type*>(value);
            }

            // 对象还没被创建, 尝试创建.
            if(Acquire_CompareAndSwap(&instance_, 0, kBeingCreatedMarker) == 0)
            {
                // instance_为NULL且已设置kBeingCreatedMarker. 只可能有一个线程在这里,
                // 其它线程只能循环等待(spinning).
                Type* newval = Traits::New();

                Release_Store(&instance_, reinterpret_cast<AtomicWord>(newval));

                if(newval!=NULL && Traits::kRegisterAtExit)
                {
                    AtExitManager::RegisterCallback(OnExit, NULL);
                }

                return newval;
            }

            // 冲突, 其它线程已经:
            // - 正在创建
            // - 已经创建完成
            // value != NULL时可能是kBeingCreatedMarker或者合法指针.
            // 一般不会冲突, 除非实例对象构造非常耗时, 此时不断循环切换到其它线程直至
            // 对象创建成功.
            while(true)
            {
                value = NoBarrier_Load(&instance_);
                if(value != kBeingCreatedMarker)
                {
                    break;
                }
                Sleep(0);
            }

            return reinterpret_cast<Type*>(value);
        }

    private:
        // AtExit()的适配函数. 应该在单线程中调用, 这个前提可能不是必须的.
        static void OnExit(void* unused)
        {
            // 单件实例创建之后才会在AtExit中注册. instance_指针非法则不会调用.
            Traits::Delete(reinterpret_cast<Type*>(
                NoBarrier_AtomicExchange(&instance_, 0)));
        }
        static AtomicWord instance_;
    };

    template<typename Type, typename Traits, typename DifferentiatingType>
    AtomicWord Singleton<Type, Traits, DifferentiatingType>::instance_ = 0;

} //namespace base

#endif //__base_singleton_h__