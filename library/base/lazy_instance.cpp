
#include "lazy_instance.h"

#include "at_exit.h"
#include "threading/platform_thread.h"

namespace base
{

    bool LazyInstanceHelper::NeedsInstance()
    {
        // Try to create the instance, if we're the first, will go from EMPTY
        // to CREATING, otherwise we've already been beaten here.
        // The memory access has no memory ordering as STATE_EMPTY and STATE_CREATING
        // has no associated data (memory barriers are all about ordering
        // of memory accesses to *associated* data).
        if(subtle::NoBarrier_CompareAndSwap(&state_, STATE_EMPTY,
            STATE_CREATING) == STATE_EMPTY)
        {
            // Caller must create instance
            return true;
        }

        // It's either in the process of being created, or already created. Spin.
        // The load has acquire memory ordering as a thread which sees
        // state_ == STATE_CREATED needs to acquire visibility over
        // the associated data (buf_). Pairing Release_Store is in
        // CompleteInstance().
        while(subtle::Acquire_Load(&state_) != STATE_CREATED)
        {
            PlatformThread::YieldCurrentThread();
        }

        // Someone else created the instance.
        return false;
    }

    void LazyInstanceHelper::CompleteInstance(void* instance, void (*dtor)(void*))
    {
        // Instance is created, go from CREATING to CREATED.
        // Releases visibility over buf_ to readers. Pairing Acquire_Load's are in
        // NeedsInstance() and Pointer().
        subtle::Release_Store(&state_, STATE_CREATED);

        // Make sure that the lazily instantiated object will get destroyed at exit.
        if(dtor)
        {
            AtExitManager::RegisterCallback(dtor, instance);
        }
    }

} //namespace base