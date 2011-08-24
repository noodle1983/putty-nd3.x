
// 引用计数原子操作语义的底层实现. 请直接使用memory/ref_counted.h.

#ifndef __base_atomic_ref_count_h__
#define __base_atomic_ref_count_h__

#pragma once

#include "atomicops.h"

namespace base
{

    typedef subtle::Atomic32 AtomicRefCount;

    // 增加引用计数, "increment"必须大于0.
    inline void AtomicRefCountIncN(volatile AtomicRefCount* ptr,
        AtomicRefCount increment)
    {
        subtle::NoBarrier_AtomicIncrement(ptr, increment);
    }

    // 减少引用计数, "decrement"必须大于0, 返回结果是否非0.
    inline bool AtomicRefCountDecN(volatile AtomicRefCount* ptr,
        AtomicRefCount decrement)
    {
        bool res = subtle::Barrier_AtomicIncrement(ptr, -decrement) != 0;
        return res;
    }

    // 引用计数加1.
    inline void AtomicRefCountInc(volatile AtomicRefCount* ptr)
    {
        AtomicRefCountIncN(ptr, 1);
    }

    // 引用计数减1, 返回结果是否非0.
    inline bool AtomicRefCountDec(volatile AtomicRefCount* ptr)
    {
        return AtomicRefCountDecN(ptr, 1);
    }

    // 返回引用计数是否为1. 按照常规实现方式, 引用计数为1表明只有当前线程拥有引用,
    // 不与其它线程共享. 函数测试引用计数是否为1, 以确定对象是否需要互斥访问.
    inline bool AtomicRefCountIsOne(volatile AtomicRefCount* ptr)
    {
        bool res = subtle::Acquire_Load(ptr) == 1;
        return res;
    }

    // 返回引用计数是否为0. 按照常规实现方式, 引用计数为0, 对象需要销毁,
    // 一次引用计数应该不为0, 函数一般用于调试检查.
    inline bool AtomicRefCountIsZero(volatile AtomicRefCount* ptr)
    {
        bool res = subtle::Acquire_Load(ptr) == 0;
        return res;
    }

} //namespace base

#endif //__base_atomic_ref_count_h__