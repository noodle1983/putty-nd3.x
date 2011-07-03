
#include "non_thread_safe_impl.h"

#include "base/logging.h"

namespace base
{

    NonThreadSafeImpl::~NonThreadSafeImpl()
    {
        DCHECK(CalledOnValidThread());
    }

    bool NonThreadSafeImpl::CalledOnValidThread() const
    {
        return thread_checker_.CalledOnValidThread();
    }

    void NonThreadSafeImpl::DetachFromThread()
    {
        thread_checker_.DetachFromThread();
    }

} //namespace base