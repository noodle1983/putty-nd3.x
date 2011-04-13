
#include "thread_checker.h"

#ifndef NDEBUG

ThreadChecker::ThreadChecker() : valid_thread_id_(base::kInvalidThreadId)
{
    EnsureThreadIdAssigned();
}

ThreadChecker::~ThreadChecker() {}

bool ThreadChecker::CalledOnValidThread() const
{
    EnsureThreadIdAssigned();
    base::AutoLock auto_lock(lock_);
    return valid_thread_id_ == base::PlatformThread::CurrentId();
}

void ThreadChecker::DetachFromThread()
{
    base::AutoLock auto_lock(lock_);
    valid_thread_id_ = base::kInvalidThreadId;
}

void ThreadChecker::EnsureThreadIdAssigned() const
{
    base::AutoLock auto_lock(lock_);
    if(valid_thread_id_ != base::kInvalidThreadId)
    {
        return;
    }
    valid_thread_id_ = base::PlatformThread::CurrentId();
}

#endif //NDEBUG