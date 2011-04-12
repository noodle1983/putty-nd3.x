
#include "thread.h"

#include "base/lazy_instance.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_local.h"
#include "base/win/windows_version.h"

namespace base
{

    // 用于触发消息循环退出.
    class ThreadQuitTask : public Task
    {
    public:
        virtual void Run()
        {
            MessageLoop::current()->Quit();
            Thread::SetThreadWasQuitProperly(true);
        }
    };

    // 用于传递数据给ThreadMain, 在StartWithOptions函数的栈上分配对象.
    struct Thread::StartupData
    {
        // 因为在栈上分配结构体对象, 所以这里可以只用常量引用.
        const Thread::Options& options;

        // 用于同步线程启动.
        WaitableEvent event;

        explicit StartupData(const Options& opt)
            : options(opt), event(false, false) {}
    };

    Thread::Thread(const char* name)
        : started_(false),
        stopping_(false),
        startup_data_(NULL),
        thread_(0),
        message_loop_(NULL),
        thread_id_(0),
        name_(name) {}

    Thread::~Thread()
    {
        Stop();
    }

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
        ThreadDelegate* delegate;
        bool joinable;
    };

    DWORD __stdcall ThreadFunc(void* params)
    {
        ThreadParams* thread_params = static_cast<ThreadParams*>(params);
        ThreadDelegate* delegate = thread_params->delegate;
        if(!thread_params->joinable)
        {
            base::ThreadRestrictions::SetSingletonAllowed(false);
        }
        delete thread_params;
        delegate->ThreadMain();
        return NULL;
    }

    bool CreateThreadInternal(size_t stack_size, ThreadDelegate* delegate,
        HANDLE* out_thread_handle)
    {
        HANDLE thread_handle;
        unsigned int flags = 0;
        if(stack_size>0 && base::GetWinVersion()>=base::WINVERSION_XP)
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

    // static
    void Thread::SetName(const char* name)
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
        info.dwThreadID = GetCurrentThreadId();
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
    bool Thread::Create(size_t stack_size, ThreadDelegate* delegate,
        HANDLE* thread_handle)
    {
        DCHECK(thread_handle);
        return CreateThreadInternal(stack_size, delegate, thread_handle);
    }

    // static
    void Thread::Join(HANDLE thread_handle)
    {
        DCHECK(thread_handle);

        // WLW TODO: 一旦Windows关闭能正常工作就启动这个校验.
        // Joining其它线程会导致当前进程堵塞一段时间, 因为|thread_handle|线程可能还在
        // 执行一个长时间堵塞的任务.
#if 0
        base::ThreadRestrictions::AssertIOAllowed();
#endif

        // 等待线程退出. 线程应该会结束, 但需要保证这个假设是正确的.
        DWORD result = WaitForSingleObject(thread_handle, INFINITE);
        DCHECK_EQ(WAIT_OBJECT_0, result);

        CloseHandle(thread_handle);
    }

    namespace
    {

        // 使用线程局部变量记录一个线程是否通过调用Stop方法退出的. 这样我们可以捕捉到
        // 直接调用MessageLoop::Quit()的情况, 对于Thread启动并运行的MessageLoop, 不希
        // 望使用这种退出方式.
        base::LazyInstance<base::ThreadLocalBoolean> lazy_tls_bool(
            base::LINKER_INITIALIZED);

    }

    void Thread::SetThreadWasQuitProperly(bool flag)
    {
        lazy_tls_bool.Pointer()->Set(flag);
    }

    bool Thread::GetThreadWasQuitProperly()
    {
        bool quit_properly = true;
#ifndef NDEBUG
        quit_properly = lazy_tls_bool.Pointer()->Get();
#endif
        return quit_properly;
    }

    bool Thread::Start()
    {
        return StartWithOptions(Options());
    }

    bool Thread::StartWithOptions(const Options& options)
    {
        DCHECK(!message_loop_);

        SetThreadWasQuitProperly(false);

        StartupData startup_data(options);
        startup_data_ = &startup_data;

        if(!Create(options.stack_size, this, &thread_))
        {
            DLOG(ERROR) << "failed to create thread";
            startup_data_ = NULL;
            return false;
        }

        // 等待线程启动并初始化message_loop_.
        startup_data.event.Wait();

        // 设置成NULL, 所以我们不会保存一个栈上对象的指针.
        startup_data_ = NULL;
        started_ = true;

        DCHECK(message_loop_);
        return true;
    }

    void Thread::Stop()
    {
        if(!thread_was_started())
        {
            return;
        }

        StopSoon();

        // 等待线程退出.
        //
        // TODO: 很不幸, 我们需要保持message_loop_运行直到线程退出. 有些用户乱用
        // API. 使它们停止.
        Join(thread_);

        // 线程退出时会把message_loop_设置成NULL.
        DCHECK(!message_loop_);

        // 线程不需要再次结束.
        started_ = false;

        stopping_ = false;
    }

    void Thread::StopSoon()
    {
        // 只能在启动线程中调用(启动本线程的线程!=本线程).
        DCHECK_NE(thread_id_, GetCurrentThreadId());

        if(stopping_ || !message_loop_)
        {
            return;
        }

        stopping_ = true;
        message_loop_->PostTask(new ThreadQuitTask());
    }

    void Thread::Run(MessageLoop* message_loop)
    {
        message_loop->Run();
    }

    void Thread::ThreadMain()
    {
        {
            // 本线程的消息循环.
            MessageLoop message_loop(startup_data_->options.message_loop_type);

            // 完成线程对象的初始化.
            thread_id_ = GetCurrentThreadId();
            SetName(name_.c_str());
            message_loop.set_thread_name(name_);
            message_loop_ = &message_loop;
            message_loop_proxy_ = MessageLoopProxy::CreateForCurrentThread();

            // 允许线程做额外的初始化工作, 在通知启动线程前调用.
            Init();

            startup_data_->event.Signal();
            // 不能再使用startup_data_, 因为启动线程此时已经解锁.

            Run(message_loop_);

            // 允许线程做额外的清理工作.
            CleanUp();

            // 断言MessageLoop::Quit是被ThreadQuitTask调用的.
            DCHECK(GetThreadWasQuitProperly());

            // 不再接受任何消息.
            message_loop_ = NULL;
            message_loop_proxy_ = NULL;
        }
        thread_id_ = 0;
    }

} //namespace base