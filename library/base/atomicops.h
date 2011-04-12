
// reference counts方面的原子操作参见atomic_refcount.h.
// sequence numbers方面的原子操作参见atomic_sequence_num.h.
//
// 本模块暴露的例程具有一定风险. 不仅需要正确编码, 还需要对原子化(atomicity)
// 和内存有序化(memory ordering)仔细推理; 可读性和可维护性都较差. 假如不使用
// 会有严重影响或者别无选择的时候才考虑这些函数. 最好只使用有显式安全说明
// 的函数, 最好只在x86平台上使用, 其它的架构平台上会导致崩溃. 如果不能确保
// 这些, 尽量不要使用, 可以选择使用Mutex.
//
// 直接存取原子变量是错误的. 应该使用NoBarrier版本的存取函数:
//     NoBarrier_Store()
//     NoBarrier_Load()
// 当前的编译器层面没有做强制要求, 但是最好这样做.

#ifndef __base_atomicops_h__
#define __base_atomicops_h__

#pragma once

#include <windows.h>

#include "basic_types.h"

namespace base
{

    typedef __w64 int32 Atomic32;
#ifdef ARCH_CPU_64_BITS
    // 需要在Atomic64和AtomicWord之间进行隐式转换, 这意味着它们在64-bit下类型一致.
    typedef intptr_t Atomic64;
#endif

    // AtomicWord是一个机器指针长度, 是Atomic32还是Atomic64依赖于CPU架构.
    typedef intptr_t AtomicWord;

    // 原子操作:
    //     result = *ptr;
    //     if(*ptr == old_value)
    //         *ptr = new_value;
    //     return result;
    //
    // 如果"*ptr"=="old_value"则被替换成"new_value", 返回"*ptr"以前的值.
    // 没有内存屏障(memory barriers).
    Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
        Atomic32 old_value, Atomic32 new_value);

    // 原子操作: 存储new_value到*ptr, 并返回以前的值.
    // 没有内存屏障(memory barriers).
    Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr, Atomic32 new_value);

    // 原子操作: *ptr值增加"increment", 返回增加后的新值.
    // 没有内存屏障(memory barriers).
    Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr, Atomic32 increment);

    Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr, Atomic32 increment);

    // 下面几个底层函数对于开发自旋锁(spinlocks)、信号量(mutexes)和条件变量
    // (condition-variables)这种上层同步的开发者有用. 组合了CompareAndSwap(),
    // 取或存操作, 保证指令操作内存有序化. "Acquire"函数保证后面的内存访问不会
    // 提前; "Release"函数保证前面的内存访问不会延后. "Barrier"函数兼有
    // "Acquire"和"Release"语义. MemoryBarrier()具有"Barrier"语义但是没有访问
    // 内存.
    Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
        Atomic32 old_value, Atomic32 new_value);
    Atomic32 Release_CompareAndSwap(volatile Atomic32* ptr,
        Atomic32 old_value, Atomic32 new_value);

    void MemoryBarrier();
    void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value);
    void Acquire_Store(volatile Atomic32* ptr, Atomic32 value);
    void Release_Store(volatile Atomic32* ptr, Atomic32 value);

    Atomic32 NoBarrier_Load(volatile const Atomic32* ptr);
    Atomic32 Acquire_Load(volatile const Atomic32* ptr);
    Atomic32 Release_Load(volatile const Atomic32* ptr);

    // 64位原子操作(仅在64位处理器下有效).
#ifdef ARCH_CPU_64_BITS
    Atomic64 NoBarrier_CompareAndSwap(volatile Atomic64* ptr,
        Atomic64 old_value, Atomic64 new_value);
    Atomic64 NoBarrier_AtomicExchange(volatile Atomic64* ptr, Atomic64 new_value);
    Atomic64 NoBarrier_AtomicIncrement(volatile Atomic64* ptr, Atomic64 increment);
    Atomic64 Barrier_AtomicIncrement(volatile Atomic64* ptr, Atomic64 increment);

    Atomic64 Acquire_CompareAndSwap(volatile Atomic64* ptr,
        Atomic64 old_value, Atomic64 new_value);
    Atomic64 Release_CompareAndSwap(volatile Atomic64* ptr,
        Atomic64 old_value, Atomic64 new_value);
    void NoBarrier_Store(volatile Atomic64* ptr, Atomic64 value);
    void Acquire_Store(volatile Atomic64* ptr, Atomic64 value);
    void Release_Store(volatile Atomic64* ptr, Atomic64 value);
    Atomic64 NoBarrier_Load(volatile const Atomic64* ptr);
    Atomic64 Acquire_Load(volatile const Atomic64* ptr);
    Atomic64 Release_Load(volatile const Atomic64* ptr);
#endif //ARCH_CPU_64_BITS


    inline Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
        Atomic32 old_value, Atomic32 new_value)
    {
        LONG result = InterlockedCompareExchange(
            reinterpret_cast<volatile LONG*>(ptr),
            static_cast<LONG>(new_value),
            static_cast<LONG>(old_value));
        return static_cast<Atomic32>(result);
    }

    inline Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr,
        Atomic32 new_value)
    {
        LONG result = InterlockedExchange(
            reinterpret_cast<volatile LONG*>(ptr),
            static_cast<LONG>(new_value));
        return static_cast<Atomic32>(result);
    }

    inline Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr,
        Atomic32 increment)
    {
        return Barrier_AtomicIncrement(ptr, increment);
    }

    inline Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr,
        Atomic32 increment)
    {
        return InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(ptr),
            static_cast<LONG>(increment))+increment;
    }

    inline Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
        Atomic32 old_value, Atomic32 new_value)
    {
        return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
    }

    inline Atomic32 Release_CompareAndSwap(volatile Atomic32* ptr,
        Atomic32 old_value, Atomic32 new_value)
    {
        return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
    }

    inline void MemoryBarrier()
    {
        ::MemoryBarrier();
    }

    inline void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value)
    {
        *ptr = value;
    }

    inline void Acquire_Store(volatile Atomic32* ptr, Atomic32 value)
    {
        NoBarrier_AtomicExchange(ptr, value);
    }

    inline void Release_Store(volatile Atomic32* ptr, Atomic32 value)
    {
        *ptr = value;
    }

    inline Atomic32 NoBarrier_Load(volatile const Atomic32* ptr)
    {
        return *ptr;
    }

    inline Atomic32 Acquire_Load(volatile const Atomic32* ptr)
    {
        Atomic32 value = *ptr;
        return value;
    }

    inline Atomic32 Release_Load(volatile const Atomic32* ptr)
    {
        MemoryBarrier();
        return *ptr;
    }

#if defined(_WIN64)
    // 64位平台下的64位低级操作
    COMPILE_ASSERT(sizeof(Atomic64)==sizeof(PVOID), atomic_word_is_atomic);

    inline Atomic64 NoBarrier_CompareAndSwap(volatile Atomic64* ptr,
        Atomic64 old_value, Atomic64 new_value)
    {
        PVOID result = InterlockedCompareExchangePointer(
            reinterpret_cast<volatile PVOID*>(ptr),
            reinterpret_cast<PVOID>(new_value), reinterpret_cast<PVOID>(old_value));
        return reinterpret_cast<Atomic64>(result);
    }

    inline Atomic64 NoBarrier_AtomicExchange(volatile Atomic64* ptr,
        Atomic64 new_value)
    {
        PVOID result = InterlockedExchangePointer(
            reinterpret_cast<volatile PVOID*>(ptr),
            reinterpret_cast<PVOID>(new_value));
        return reinterpret_cast<Atomic64>(result);
    }

    inline Atomic64 Barrier_AtomicIncrement(volatile Atomic64* ptr,
        Atomic64 increment)
    {
        return InterlockedExchangeAdd64(
            reinterpret_cast<volatile LONGLONG*>(ptr),
            static_cast<LONGLONG>(increment)) + increment;
    }

    inline Atomic64 NoBarrier_AtomicIncrement(volatile Atomic64* ptr,
        Atomic64 increment)
    {
        return Barrier_AtomicIncrement(ptr, increment);
    }

    inline void NoBarrier_Store(volatile Atomic64* ptr, Atomic64 value)
    {
        *ptr = value;
    }

    inline void Acquire_Store(volatile Atomic64* ptr, Atomic64 value)
    {
        NoBarrier_AtomicExchange(ptr, value);
    }

    inline void Release_Store(volatile Atomic64* ptr, Atomic64 value)
    {
        *ptr = value;
    }

    inline Atomic64 NoBarrier_Load(volatile const Atomic64* ptr)
    {
        return *ptr;
    }

    inline Atomic64 Acquire_Load(volatile const Atomic64* ptr)
    {
        Atomic64 value = *ptr;
        return value;
    }

    inline Atomic64 Release_Load(volatile const Atomic64* ptr)
    {
        MemoryBarrier();
        return *ptr;
    }

    inline Atomic64 Acquire_CompareAndSwap(volatile Atomic64* ptr,
        Atomic64 old_value, Atomic64 new_value)
    {
        return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
    }

    inline Atomic64 Release_CompareAndSwap(volatile Atomic64* ptr,
        Atomic64 old_value, Atomic64 new_value)
    {
        return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
    }
#endif //defined(_WIN64)

} //namespace base

#endif //__base_atomicops_h__