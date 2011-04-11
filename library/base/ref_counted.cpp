
#include "ref_counted.h"

#include "logging.h"

namespace base
{

    RefCountedBase::RefCountedBase() : ref_count_(0)
#ifndef NDEBUG
        , in_dtor_(false)
#endif
    {}

    RefCountedBase::~RefCountedBase()
    {
#ifndef NDEBUG
        DCHECK(in_dtor_) << "RefCounted object deleted without "
            "calling Release()";
#endif
    }

    void RefCountedBase::AddRef() const
    {
#ifndef NDEBUG
        DCHECK(!in_dtor_);
#endif
        ++ref_count_;
    }

    bool RefCountedBase::Release() const
    {
#ifndef NDEBUG
        DCHECK(!in_dtor_);
#endif
        if(--ref_count_ == 0)
        {
#ifndef NDEBUG
            in_dtor_ = true;
#endif
            return true;
        }
        return false;
    }

    RefCountedThreadSafeBase::RefCountedThreadSafeBase() : ref_count_(0)
    {
#ifndef NDEBUG
        in_dtor_ = false;
#endif
    }

    RefCountedThreadSafeBase::~RefCountedThreadSafeBase()
    {
#ifndef NDEBUG
        DCHECK(in_dtor_) << "RefCountedThreadSafe object deleted without "
            "calling Release()";
#endif
    }

    void RefCountedThreadSafeBase::AddRef() const
    {
#ifndef NDEBUG
        DCHECK(!in_dtor_);
#endif
        AtomicRefCountInc(&ref_count_);
    }

    bool RefCountedThreadSafeBase::Release() const
    {
#ifndef NDEBUG
        DCHECK(!in_dtor_);
        DCHECK(!AtomicRefCountIsZero(&ref_count_));
#endif
        if(!AtomicRefCountDec(&ref_count_))
        {
#ifndef NDEBUG
            in_dtor_ = true;
#endif
            return true;
        }
        return false;
    }

    bool RefCountedThreadSafeBase::HasOneRef() const
    {
        return AtomicRefCountIsOne(
            &const_cast<RefCountedThreadSafeBase*>(this)->ref_count_);
    }

} //namespace base