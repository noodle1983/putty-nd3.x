
#include "lock_impl.h"

namespace base
{
    namespace internal
    {

        LockImpl::LockImpl()
        {
            // 第二个参数是自旋技术, 对于短锁的情况可以避免线程切换提高性能.
            // MSDN说: 第二个参数在多处理器情况下, 一个线程在请求已加锁的
            //         critical section时会进入循环, 当超时后才会进入sleep.
            InitializeCriticalSectionAndSpinCount(&os_lock_, 2000);
        }

        LockImpl::~LockImpl()
        {
            DeleteCriticalSection(&os_lock_);
        }

        bool LockImpl::Try()
        {
            if(TryEnterCriticalSection(&os_lock_) != FALSE)
            {
                return true;
            }
            return false;
        }

        void LockImpl::Lock()
        {
            EnterCriticalSection(&os_lock_);
        }

        void LockImpl::Unlock()
        {
            LeaveCriticalSection(&os_lock_);
        }

    } //namespace internal
} //namespace base