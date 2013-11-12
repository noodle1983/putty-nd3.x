
#include "thread.h"

#include "base/lazy_instance.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_local.h"
#include "base/win/windows_version.h"

#if defined(OS_WIN)
#include "base/win/scoped_com_initializer.h"
#include "base/memory/scoped_ptr.h"
#endif

namespace base
{

    namespace
    {

        // 使用线程局部变量记录一个线程是否通过调用Stop方法退出的. 这样我们可以捕捉到
        // 直接调用MessageLoop::Quit()的情况, 对于Thread启动并运行的MessageLoop, 不希
        // 望使用这种退出方式.
        base::LazyInstance<base::ThreadLocalBoolean> lazy_tls_bool(
            base::LINKER_INITIALIZED);

    }

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
#if defined(OS_WIN)
      com_status_(NONE),
#endif
        stopping_(false),
        startup_data_(NULL),
        thread_(0),
        message_loop_(NULL),
        thread_id_(kInvalidThreadId),
        name_(name) {}

    Thread::~Thread()
    {
        Stop();
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
		  Options options;
#if defined(OS_WIN)
	  if (com_status_ == STA)
		options.message_loop_type = MessageLoop::TYPE_UI;
#endif
	  return StartWithOptions(options);
    }

    bool Thread::StartWithOptions(const Options& options)
    {
        DCHECK(!message_loop_);

        SetThreadWasQuitProperly(false);

        StartupData startup_data(options);
        startup_data_ = &startup_data;

        if(!PlatformThread::Create(options.stack_size, this, &thread_))
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
        PlatformThread::Join(thread_);

        // 线程退出时会把message_loop_设置成NULL.
        DCHECK(!message_loop_);

        // 线程不需要再次结束.
        started_ = false;

        stopping_ = false;
    }

    void Thread::StopSoon()
    {
        // 只能在启动线程中调用(启动本线程的线程!=本线程).
        DCHECK_NE(thread_id_, PlatformThread::CurrentId());

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
            thread_id_ = PlatformThread::CurrentId();
            PlatformThread::SetName(name_.c_str());
            message_loop.set_thread_name(name_);
            message_loop_ = &message_loop;

#if defined(OS_WIN)
    scoped_ptr<win::ScopedCOMInitializer> com_initializer;
    if (com_status_ != NONE) {
      com_initializer.reset((com_status_ == STA) ?
          new win::ScopedCOMInitializer() :
          new win::ScopedCOMInitializer(win::ScopedCOMInitializer::kMTA));
    }
#endif
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
        }
        thread_id_ = kInvalidThreadId;
    }

} //namespace base