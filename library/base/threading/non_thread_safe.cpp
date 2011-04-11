
#include "non_thread_safe.h"

#include "../logging.h"

#ifndef NDEBUG

NonThreadSafe::~NonThreadSafe()
{
    DCHECK(CalledOnValidThread());
}

bool NonThreadSafe::CalledOnValidThread() const
{
    return thread_checker_.CalledOnValidThread();
}

void NonThreadSafe::DetachFromThread()
{
    thread_checker_.DetachFromThread();
}

#endif //NDEBUG