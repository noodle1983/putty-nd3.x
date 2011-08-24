
#ifndef __base_lock_impl_h__
#define __base_lock_impl_h__

#pragma once

#include <windows.h>

#include "base/basic_types.h"

namespace base
{
    namespace internal
    {

        // LockImpl的实现依赖操作系统的自旋锁机制.
        // 一般情况下不要直接使用LockImpl, 请使用Lock.
        class LockImpl
        {
        public:
            typedef CRITICAL_SECTION OSLockType;

            LockImpl();
            ~LockImpl();

            // 如果当前没有锁, 上锁并返回true, 否则立即返回错误.
            bool Try();
            // 加锁, 函数堵塞直到加锁成功.
            void Lock();
            // 解锁. 只能由上锁者调用: Try()成功返回或者Lock()成功后.
            void Unlock();

        private:
            OSLockType os_lock_;

            DISALLOW_COPY_AND_ASSIGN(LockImpl);
        };

    } //namespace internal
} //namespace base

#endif //__base_lock_impl_h__