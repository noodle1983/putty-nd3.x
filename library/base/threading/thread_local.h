
// 警告: 线程局部存储用好不是那么容易, 请确保这就是你想要的最好方案. 不要
// 过早的优化, 比如用一个Lock即可达到目的.
//
// 这些类对操作系统的TLS存储机制进行了封装. 在构造函数中分配一个TLS插槽,
// 析构的时候释放, 没有其它的内存管理. 这意味着使用ThreadLocalPointer时,
// 必须正确的管理自己申请的内存. 这些类不会帮你销毁指针, 在线程退出时没
// 有任何额外操作.
//
// ThreadLocalPointer<Type>封装Type*, 不负责创建和销毁, 所以内存必须在
// 其它地方管理正确. 在一个线程首次调用Get()会返回NULL, 需要通过Set()
// 调用设置指针.
//
// ThreadLocalBoolean封装bool, 缺省是false, 需要通过Set()调用设置值.
//
// 线程安全性: ThreadLocalStorage实例在创建之后是线程安全的. 如果想动态
// 创建实例, 必须处理好线程冲突. 也就是说函数级别的静态初始化是不太好的.
//
// 使用示例:
//     // MyClass类附加在一个线程上. 存储指针到TLS, 这样就能实现current().
//     MyClass::MyClass() {
//       DCHECK(Singleton<ThreadLocalPointer<MyClass> >::get()->Get() == NULL);
//       Singleton<ThreadLocalPointer<MyClass> >::get()->Set(this);
//     }
//
//     MyClass::~MyClass() {
//       DCHECK(Singleton<ThreadLocalPointer<MyClass> >::get()->Get() != NULL);
//       Singleton<ThreadLocalPointer<MyClass> >::get()->Set(NULL);
//     }
//
//     // Return the current MyClass associated with the calling thread, can be
//     // NULL if there isn't a MyClass associated.
//     MyClass* MyClass::current() {
//       return Singleton<ThreadLocalPointer<MyClass> >::get()->Get();
//     }

#ifndef __base_threading_thread_local_h__
#define __base_threading_thread_local_h__

#pragma once

#include "../basic_types.h"

namespace base
{

    // 跨平台APIs抽象函数. 不要直接使用.
    struct ThreadLocalPlatform
    {
        typedef unsigned long SlotType;

        static void AllocateSlot(SlotType& slot);
        static void FreeSlot(SlotType& slot);
        static void* GetValueFromSlot(SlotType& slot);
        static void SetValueInSlot(SlotType& slot, void* value);
    };

    template<typename Type>
    class ThreadLocalPointer
    {
    public:
        ThreadLocalPointer() : slot_()
        {
            ThreadLocalPlatform::AllocateSlot(slot_);
        }

        ~ThreadLocalPointer()
        {
            ThreadLocalPlatform::FreeSlot(slot_);
        }

        Type* Get()
        {
            return static_cast<Type*>(ThreadLocalPlatform::GetValueFromSlot(slot_));
        }

        void Set(Type* ptr)
        {
            ThreadLocalPlatform::SetValueInSlot(slot_, ptr);
        }

    private:
        typedef ThreadLocalPlatform::SlotType SlotType;

        SlotType slot_;

        DISALLOW_COPY_AND_ASSIGN(ThreadLocalPointer<Type>);
    };

    class ThreadLocalBoolean
    {
    public:
        ThreadLocalBoolean() {}
        ~ThreadLocalBoolean() {}

        bool Get()
        {
            return tlp_.Get() != NULL;
        }

        void Set(bool val)
        {
            tlp_.Set(reinterpret_cast<void*>(val ? 1 : 0));
        }

    private:
        ThreadLocalPointer<void> tlp_;

        DISALLOW_COPY_AND_ASSIGN(ThreadLocalBoolean);
    };

} //namespace base

#endif //__base_threading_thread_local_h__