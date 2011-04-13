
#ifndef __base_synchronization_lock_h__
#define __base_synchronization_lock_h__

#pragma once

#include "../threading/platform_thread.h"
#include "lock_impl.h"

namespace base
{

    // OS临界区封装. 只有debug模式下支持AssertAcquired()方法.
    class Lock
    {
    public:
#if defined(NDEBUG)
        Lock() : lock_() {}
        ~Lock() {}
        void Acquire() { lock_.Lock(); }
        void Release() { lock_.Unlock(); }

        // 如果没有上锁, 上锁并返回true. 如果已经被其它线程上锁, 立即返回false.
        // 已经上锁的线程不要再调用(可能产生预期不到的断言失败).
        bool Try() { return lock_.Try(); }

        // 非debug模式下是空实现.
        void AssertAcquired() const {}
#else //!NDEBUG
        Lock();
        ~Lock() {}

        // 注意: 尽管windows的临界区支持递归加锁, 但是这里不允许, 如果线程第二次
        // 请求加锁(已经加锁)会触发DCHECK().
        void Acquire()
        {
            lock_.Lock();
            CheckUnheldAndMark();
        }
        void Release()
        {
            CheckHeldAndUnmark();
            lock_.Unlock();
        }

        bool Try()
        {
            bool rv = lock_.Try();
            if(rv)
            {
                CheckUnheldAndMark();
            }
            return rv;
        }

        void AssertAcquired() const;
#endif //NDEBUG

    private:
#if !defined(NDEBUG)
        // 下面2个函数为递归加锁时触发断言设计.
        void CheckHeldAndUnmark();
        void CheckUnheldAndMark();

        // 所有私有成员受lock_保护, 因此需要注意只能在加锁后使用.

        // owned_by_thread_用来确定owning_thread_id_是否合法.
        // 因为owning_thread_id_没有空值(null).
        bool owned_by_thread_;
        base::PlatformThreadId owning_thread_id_;
#endif //NDEBUG

        LockImpl lock_;

        DISALLOW_COPY_AND_ASSIGN(Lock);
    };

    // 自动加锁辅助类, 在作用域内自动加锁.
    class AutoLock
    {
    public:
        explicit AutoLock(Lock& lock) : lock_(lock)
        {
            lock_.Acquire();
        }

        ~AutoLock()
        {
            lock_.AssertAcquired();
            lock_.Release();
        }

    private:
        Lock& lock_;
        DISALLOW_COPY_AND_ASSIGN(AutoLock);
    };

    // 自动解锁辅助类, 构造函数中断言已加锁并解锁, 析构时重新加锁.
    class AutoUnlock
    {
    public:
        explicit AutoUnlock(Lock& lock) : lock_(lock)
        {
            // 断言调用者已加锁.
            lock_.AssertAcquired();
            lock_.Release();
        }

        ~AutoUnlock()
        {
            lock_.Acquire();
        }

    private:
        Lock& lock_;
        DISALLOW_COPY_AND_ASSIGN(AutoUnlock);
    };

} //namespace base

#endif //__base_synchronization_lock_h__