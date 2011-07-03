
#include "thread_checker_impl.h"

namespace base
{

    ThreadCheckerImpl::ThreadCheckerImpl()
        : valid_thread_id_(kInvalidThreadId)
    {
        EnsureThreadIdAssigned();
    }

    ThreadCheckerImpl::~ThreadCheckerImpl() {}

    bool ThreadCheckerImpl::CalledOnValidThread() const
    {
        EnsureThreadIdAssigned();
        AutoLock auto_lock(lock_);
        return valid_thread_id_ == PlatformThread::CurrentId();
    }

    void ThreadCheckerImpl::DetachFromThread()
    {
        AutoLock auto_lock(lock_);
        valid_thread_id_ = kInvalidThreadId;
    }

    void ThreadCheckerImpl::EnsureThreadIdAssigned() const
    {
        AutoLock auto_lock(lock_);
        if(valid_thread_id_ != kInvalidThreadId)
        {
            return;
        }
        valid_thread_id_ = PlatformThread::CurrentId();
    }

} //namespace base