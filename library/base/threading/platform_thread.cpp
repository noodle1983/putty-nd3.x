
#include "platform_thread.h"

#include "../logging.h"
#include "../win/windows_version.h"
#include "thread_restrictions.h"

namespace base
{

    namespace
    {

        // 如何设置线程名字的信息来自一篇MSDN文档:
        //   http://msdn2.microsoft.com/en-us/library/xcb2z8hs.aspx
        const DWORD kVCThreadNameException = 0x406D1388;

        typedef struct tagTHREADNAME_INFO
        {
            DWORD dwType;     // 必须是0x1000.
            LPCSTR szName;    // 指向名字(在用户地址空间).
            DWORD dwThreadID; // 线程ID(-1=调用者线程).
            DWORD dwFlags;    // 保留以便将来使用, 必须为0.
        } THREADNAME_INFO;

        struct ThreadParams
        {
            PlatformThread::Delegate* delegate;
            bool joinable;
        };

        DWORD __stdcall ThreadFunc(void* params)
        {
            ThreadParams* thread_params = static_cast<ThreadParams*>(params);
            PlatformThread::Delegate* delegate = thread_params->delegate;
            if(!thread_params->joinable)
            {
                ThreadRestrictions::SetSingletonAllowed(false);
            }
            delete thread_params;
            delegate->ThreadMain();
            return NULL;
        }

        bool CreateThreadInternal(size_t stack_size,
            PlatformThread::Delegate* delegate,
            PlatformThreadHandle* out_thread_handle)
        {
            PlatformThreadHandle thread_handle;
            unsigned int flags = 0;
            if(stack_size>0 && GetWinVersion()>=WINVERSION_XP)
            {
                flags = STACK_SIZE_PARAM_IS_A_RESERVATION;
            }
            else
            {
                stack_size = 0;
            }

            ThreadParams* params = new ThreadParams;
            params->delegate = delegate;
            params->joinable = out_thread_handle != NULL;

            // 这里用CreateThread而不是_beginthreadex, 可以使线程创建更快一些, 且不需要
            // 加载器上锁. 我们的代码只能在CreateThread()的线程中运行, 因为我们在Windows
            // 的线程池中执行代码. 这两种方式的差异:
            //   http://www.microsoft.com/msj/1099/win32/win321099.aspx
            thread_handle = CreateThread(NULL, stack_size, ThreadFunc, params,
                flags, NULL);
            if(!thread_handle)
            {
                delete params;
                return false;
            }

            if(out_thread_handle)
            {
                *out_thread_handle = thread_handle;
            }
            else
            {
                CloseHandle(thread_handle);
            }
            return true;
        }

    }

    // static
    PlatformThreadId PlatformThread::CurrentId()
    {
        return GetCurrentThreadId();
    }

    // static
    void PlatformThread::YieldCurrentThread()
    {
        ::Sleep(0);
    }

    // static
    void PlatformThread::Sleep(int duration_ms)
    {
        ::Sleep(duration_ms);
    }

    // static
    void PlatformThread::SetName(const char* name)
    {
        // 在异常时捕获线程名字需要有调试器存在. 如果没有调试器, 在抛出异常时也不
        // 需要线程名字.
        if(!::IsDebuggerPresent())
        {
            return;
        }

        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name;
        info.dwThreadID = CurrentId();
        info.dwFlags = 0;

        __try
        {
            RaiseException(kVCThreadNameException, 0, sizeof(info)/sizeof(DWORD),
                reinterpret_cast<DWORD_PTR*>(&info));
        }
        __except(EXCEPTION_CONTINUE_EXECUTION)
        {
        }
    }

    // static
    bool PlatformThread::Create(size_t stack_size, Delegate* delegate,
        PlatformThreadHandle* thread_handle)
    {
        DCHECK(thread_handle);
        return CreateThreadInternal(stack_size, delegate, thread_handle);
    }

    // static
    bool PlatformThread::CreateNonJoinable(size_t stack_size, Delegate* delegate)
    {
        return CreateThreadInternal(stack_size, delegate, NULL);
    }

    // static
    void PlatformThread::Join(PlatformThreadHandle thread_handle)
    {
        DCHECK(thread_handle);

        // WLW TODO: 一旦Windows关闭能正常工作就启动这个校验.
        // Joining其它线程会导致当前进程堵塞一段时间, 因为|thread_handle|线程可能还在
        // 执行一个长时间堵塞的任务.
#if 0
        ThreadRestrictions::AssertIOAllowed();
#endif

        // 等待线程退出. 线程应该会结束, 但需要保证这个假设是正确的.
        DWORD result = WaitForSingleObject(thread_handle, INFINITE);
        DCHECK_EQ(WAIT_OBJECT_0, result);

        CloseHandle(thread_handle);
    }

} //namespace base