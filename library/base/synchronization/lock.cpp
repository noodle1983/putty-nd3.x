
#include "lock.h"

#include "base/logging.h"

namespace base
{

#if !defined(NDEBUG)
    Lock::Lock() : lock_()
    {
        owned_by_thread_ = false;
        owning_thread_id_ = base::kInvalidThreadId;
    }

    void Lock::AssertAcquired() const
    {
        DCHECK(owned_by_thread_);
        DCHECK_EQ(owning_thread_id_, PlatformThread::CurrentId());
    }

    void Lock::CheckHeldAndUnmark()
    {
        DCHECK(owned_by_thread_);
        DCHECK_EQ(owning_thread_id_, PlatformThread::CurrentId());
        owned_by_thread_ = false;
        owning_thread_id_ = base::kInvalidThreadId;
    }

    void Lock::CheckUnheldAndMark()
    {
        DCHECK(!owned_by_thread_);
        owned_by_thread_ = true;
        owning_thread_id_ = PlatformThread::CurrentId();
    }
#endif  //NDEBUG

} //namespace base