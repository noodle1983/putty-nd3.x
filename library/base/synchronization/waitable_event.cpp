
#include "waitable_event.h"

#include <math.h>

#include "base/logging.h"
#include "base/time.h"

namespace base
{

    WaitableEvent::WaitableEvent(bool manual_reset, bool signaled)
        : handle_(CreateEvent(NULL, manual_reset, signaled, NULL))
    {
        // 如果发生NULL的情况程序会崩溃, 所有这里的崩溃可以让堆栈报告更多信息.
        CHECK(handle_);
    }

    WaitableEvent::WaitableEvent(HANDLE handle) : handle_(handle)
    {
        CHECK(handle) << "Tried to create WaitableEvent from NULL handle";
    }

    WaitableEvent::~WaitableEvent()
    {
        CloseHandle(handle_);
    }

    HANDLE WaitableEvent::Release()
    {
        HANDLE rv = handle_;
        handle_ = INVALID_HANDLE_VALUE;
        return rv;
    }

    void WaitableEvent::Reset()
    {
        ResetEvent(handle_);
    }

    void WaitableEvent::Signal()
    {
        SetEvent(handle_);
    }

    bool WaitableEvent::IsSignaled()
    {
        return TimedWait(TimeDelta::FromMilliseconds(0));
    }

    bool WaitableEvent::Wait()
    {
        DWORD result = WaitForSingleObject(handle_, INFINITE);
        // 这里发生失败是最不可预见的. 帮助用户知道是否发生过失败.
        DCHECK(result==WAIT_OBJECT_0) << "WaitForSingleObject failed";
        return result==WAIT_OBJECT_0;
    }

    bool WaitableEvent::TimedWait(const TimeDelta& max_time)
    {
        DCHECK(max_time >= TimeDelta::FromMicroseconds(0));
        // 这里需要小心. TimeDelta精度是微秒, 但这里只需要毫秒值. 如果剩余5.5ms, 应该
        // 延迟5ms还是6ms呢? 为避免过早的执行延迟任务, 应该是6ms.
        double timeout = ceil(max_time.InMillisecondsF());
        DWORD result = WaitForSingleObject(handle_, static_cast<DWORD>(timeout));
        switch(result)
        {
        case WAIT_OBJECT_0:
            return true;
        case WAIT_TIMEOUT:
            return false;
        }
        // 这里发生失败是最不可预见的. 帮助用户知道是否发生过失败.
        NOTREACHED() << "WaitForSingleObject failed";
        return false;
    }

    // static
    size_t WaitableEvent::WaitMany(WaitableEvent** events, size_t count)
    {
        HANDLE handles[MAXIMUM_WAIT_OBJECTS];
        CHECK_LE(count, static_cast<size_t>(MAXIMUM_WAIT_OBJECTS)) << "Can only wait on "
            << MAXIMUM_WAIT_OBJECTS << " with WaitMany";

        for(size_t i=0; i<count; ++i)
        {
            handles[i] = events[i]->handle();
        }

        // 强制是安全的, 因为count很小, 参见上面的CHECK.
        DWORD result = WaitForMultipleObjects(static_cast<DWORD>(count),
            handles,
            FALSE,      // 不是等待所有对象.
            INFINITE);  // 没有超时.
        if(result >= WAIT_OBJECT_0+count)
        {
            NOTREACHED() << "WaitForMultipleObjects failed: " << GetLastError();
            return 0;
        }

        return result - WAIT_OBJECT_0;
    }

} //namespace base