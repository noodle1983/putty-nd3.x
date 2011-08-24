
#ifndef __base_platform_thread_h__
#define __base_platform_thread_h__

#pragma once

#include <windows.h>

#include "base/basic_types.h"

namespace base
{

    // PlatformThreadHandle should not be assumed to be a numeric type, since the
    // standard intends to allow pthread_t to be a structure.  This means you
    // should not initialize it to a value, like 0.  If it's a member variable, the
    // constructor can safely "value initialize" using () in the initializer list.
    typedef DWORD PlatformThreadId;
    typedef void* PlatformThreadHandle; // HANDLE
    const PlatformThreadHandle kNullThreadHandle = NULL;

    const PlatformThreadId kInvalidThreadId = 0;

    // Valid values for SetThreadPriority()
    enum ThreadPriority
    {
        kThreadPriority_Normal,
        // Suitable for low-latency, glitch-resistant audio.
        kThreadPriority_RealtimeAudio
    };

    // A namespace for low-level thread functions.
    class PlatformThread
    {
    public:
        // 实现本接口, 在后台线程中执行代码. 新创建的线程会调用ThreadMain方法.
        class Delegate
        {
        public:
            virtual ~Delegate() {}
            virtual void ThreadMain() = 0;
        };

        // Gets the current thread id, which may be useful for logging purposes.
        static PlatformThreadId CurrentId();

        // Yield the current thread so another thread can be scheduled.
        static void YieldCurrentThread();

        // Sleeps for the specified duration (units are milliseconds).
        static void Sleep(int duration_ms);

        // 设置对调试器可见的线程名字. 没有调试器的话, 不做任何事情.
        static void SetName(const char* name);

        // Gets the thread name, if previously set by SetName.
        static const char* GetName();

        // 创建一个新线程. |stack_size|参数可以为0, 表示使用缺省的栈空间大小. 如果
        // 成功, |*thread_handle|被赋值为新创建的线程句柄, |delegate|的ThreadMain
        // 将会在新线程中执行.
        // 注意: 线程句柄不再使用时, 必须调用Join方法释放线程相关的系统资源. 确保
        // Delegate对象在线程退出前存在.
        static bool Create(size_t stack_size, Delegate* delegate,
            PlatformThreadHandle* thread_handle);

        // CreateNonJoinable() does the same thing as Create() except the thread
        // cannot be Join()'d.  Therefore, it also does not output a
        // PlatformThreadHandle.
        static bool CreateNonJoinable(size_t stack_size, Delegate* delegate);

        // 结束一个通过Create方法创建的线程. 函数调用堵塞直到目标线程退出. 会导致
        // |thread_handle|非法.
        static void Join(PlatformThreadHandle thread_handle);

        static void SetThreadPriority(PlatformThreadHandle handle,
            ThreadPriority priority);

    private:
        DISALLOW_IMPLICIT_CONSTRUCTORS(PlatformThread);
    };

} //namespace base

#endif //__base_platform_thread_h__