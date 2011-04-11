
#include "thread_checker.h"

#ifndef NDEBUG

ThreadChecker::ThreadChecker() : valid_thread_id_(0)
{
    EnsureThreadIdAssigned();
}

ThreadChecker::~ThreadChecker() {}

bool ThreadChecker::CalledOnValidThread() const
{
    EnsureThreadIdAssigned();
    base::AutoLock auto_lock(lock_);
    return valid_thread_id_ == ::GetCurrentThreadId();
}

void ThreadChecker::DetachFromThread()
{
    base::AutoLock auto_lock(lock_);
    valid_thread_id_ = 0;
}

void ThreadChecker::EnsureThreadIdAssigned() const
{
    base::AutoLock auto_lock(lock_);
    if(valid_thread_id_ != 0)
    {
        return;
    }
    valid_thread_id_ = ::GetCurrentThreadId();
}

#endif //NDEBUG