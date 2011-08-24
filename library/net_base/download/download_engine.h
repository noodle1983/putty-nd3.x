
#include "base/memory/scoped_ptr.h"
#include "base/threading/non_thread_safe.h"
#include "base/threading/thread.h"

#include "../net_base_sdk.h"

class DownloadEngine : public IDownloadEngine, public base::NonThreadSafe
{
public:
    DownloadEngine();
    virtual ~DownloadEngine();

    // IDownloadEngine µœ÷:
    virtual void Shutdown();
    virtual void Stop();
    virtual void AddTask(__int64 id, const wchar_t* const* urls,
        int urls_count);

private:
    scoped_ptr<base::Thread> main_thread_;
};