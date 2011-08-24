
#ifndef __net_base_sdk_h__
#define __net_base_sdk_h__

#ifdef NET_BASE_SDK_EXPORT
#define NET_BASE_PUBLIC __declspec(dllexport)
#else
#define NET_BASE_PUBLIC __declspec(dllimport)
#endif

// 下载引擎


// 下载引擎接口, 所有方法均非线程安全, 必须在调用StartDownloadEngine
// 函数所在线程执行.
class IDownloadEngine
{
public:
    // 关闭下载引擎, 对象销毁.
    virtual void Shutdown() = 0;
    // 关闭所有连接, 对象仍有效.
    virtual void Stop() = 0;

    virtual void AddTask(__int64 id, const wchar_t* const* urls,
        int urls_count) = 0;
};

// 启动引擎, 成功返回下载引擎对象, 失败返回NULL.
NET_BASE_PUBLIC IDownloadEngine* StartDownloadEngine();

#endif //__net_base_sdk_h__