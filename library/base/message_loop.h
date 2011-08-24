
#ifndef __base_message_loop_h__
#define __base_message_loop_h__

#pragma once

#include <queue>
#include <string>

#include "callback.h"
#include "message_loop_proxy.h"
#include "message_pump_win.h"
#include "synchronization/lock.h"
#include "task.h"

namespace base
{
    class Histogram;
}

// MessageLoop用于处理特定线程的事件. 一个线程最多只能有一个MessageLoop.
//
// 事件至少包括PostTask提交的Task或者TimerManager管理的DelayTask. 是否会处理
// UI消息取决于MessageLoop使用的消息泵类型. Windows上的异步过程调用(如果时间
// 允许)和一系列HANDLEs的信号也可能被处理.
//
// 注意: 触发特别强调, MessageLoop的方法只能在执行Run方法的线程中调用.
//
// 注意: MessageLoop有任务重入保护. 也就一个任务被处理的时候, 第二个任务不会
// 开始, 一直到第一个任务完成. 处理一个任务时创建了内部消息泵, 可能会发生重入,
// 内部泵处理本地消息时有可能启动一个内部的任务. 内部消息泵通过对话框(
// DialogBox)、通用对话框(GetOpenFileName)、OLE函数(DoDragDrop)、打印函数(
// StartDoc)等创建.
//
//
// 需要处理内部的任务时例程:
//     bool old_state = MessageLoop::current()->NestableTasksAllowed();
//     MessageLoop::current()->SetNestableTasksAllowed(true);
//     HRESULT hr = DoDragDrop(...); // 内部执行了一个模态的消息循环.
//     MessageLoop::current()->SetNestableTasksAllowed(old_state);
//     // 处理DoDragDrop()的返回值hr.
//
// 在调用SetNestableTasksAllowed(true)前, 请确保你的任务是可重入的, 所
// 有的全局变量是稳定的且可访问.
class MessageLoop : public base::MessagePump::Delegate
{
public:
    typedef base::MessagePumpWin::Dispatcher Dispatcher;
    typedef base::MessagePumpForUI::Observer Observer;

    // MessageLoop的类型指定除了任务和时钟之外还能处理的异步事件集合.
    //
    // TYPE_DEFAULT
    //   只支持任务和时钟.
    //
    // TYPE_UI
    //   还支持本地UI消息(比如Windows消息).
    //   参见MessageLoopForUI.
    //
    // TYPE_IO
    //   还支持异步IO.  参见MessageLoopForIO.
    enum Type
    {
        TYPE_DEFAULT,
        TYPE_UI,
        TYPE_IO
    };

    // 一般来说, 不需要实例化MessageLoop对象, 而是利用当前线程的MessageLoop实例.
    explicit MessageLoop(Type type=TYPE_DEFAULT);
    virtual ~MessageLoop();

    // 返回当前线程的MessageLoop对象, 没有返回NULL.
    static MessageLoop* current();

    static void EnableHistogrammer(bool enable_histogrammer);

    typedef base::MessagePump* (MessagePumpFactory)();
    // Using the given base::MessagePumpForUIFactory to override the default
    // MessagePump implementation for 'TYPE_UI'.
    static void InitMessagePumpForUIFactory(MessagePumpFactory* factory);

    // DestructionObserver在当前MessageLoop销毁前被通知, 此时MessageLoop::current()
    // 还未变成NULL. 这是MessageLoop做最后清理工作的一个机会.
    //
    // 注意: 这个观察者被通知的过程中投递到MessageLoop的所有任务都不会运行, 但是
    // 会被删除.
    class DestructionObserver
    {
    public:
        virtual void WillDestroyCurrentMessageLoop() = 0;

    protected:
        virtual ~DestructionObserver();
    };

    // 添加一个DestructionObserver, 开始接收通知.
    void AddDestructionObserver(DestructionObserver* destruction_observer);

    // 移除一个DestructionObserver. 在DestructionObserver接收通知回调时调用本
    // 方法是安全.
    void RemoveDestructionObserver(DestructionObserver* destruction_observer);

    // "PostTask"系列函数在消息循环中将来的某个时间点异步执行任务的Run方法.
    //
    // 使用PostTask版本, 任务是按照先进先出的顺序调用, 和正常的UI消息或IO事件
    // 同时被执行. 使用PostDelayedTask版本, 任务大约'delay_ms'毫秒后才被执行.
    //
    // NonNestable版本类似, 但保证在嵌套调用的MessageLoop::Run中从不派发任务,
    // 而是延迟任务到顶层MessageLoop::Run执行时派发.
    //
    // MessageLoop接管Task的所有权, 任务执行Run()方法之后会被删除.
    //
    // 注意: 这些方法可以在任意线程中调用. Task只会在执行MessageLoop::Run()
    // 的线程中调用.
    void PostTask(Task* task);
    void PostDelayedTask(Task* task, int64 delay_ms);
    void PostNonNestableTask(Task* task);
    void PostNonNestableDelayedTask(Task* task, int64 delay_ms);

    // TODO(ajwong): Remove the functions above once the Task -> Closure migration
    // is complete.
    //
    // There are 2 sets of Post*Task functions, one which takes the older Task*
    // function object representation, and one that takes the newer base::Closure.
    // We have this overload to allow a staged transition between the two systems.
    // Once the transition is done, the functions above should be deleted.
    void PostTask(const base::Closure& task);
    void PostDelayedTask(const base::Closure& task, int64 delay_ms);
    void PostNonNestableTask(const base::Closure& task);
    void PostNonNestableDelayedTask(const base::Closure& task, int64 delay_ms);

    // 一种删除指定对象的PostTask, 当对象需要存活到MessageLoop的下一次循环时会
    // 用到(比如在IPC回调时直接删除RenderProcessHost并不好).
    //
    // 注意: 方法可以在任意线程中调用. 对象将在执行MessageLoop::Run()的线程中
    // 被删除, 如果和调用PostDelayedTask()的线程不是同一线程, 那么T必须继承自
    // RefCountedThreadSafe<T>!
    template<class T>
    void DeleteSoon(T* object)
    {
        PostNonNestableTask(new DeleteTask<T>(object));
    }

    // 一种释放指定对象引用计数(通过调用Release方法)的PostTask, 当对象需要存活
    // 到MessageLoop的下一次循环时会或者对象需要在特定线程中释放时会用到.
    //
    // 注意: 方法可以在任意线程中调用. 对象将在执行MessageLoop::Run()的线程中
    // 释放引用(可能会被删除), 如果和调用PostDelayedTask()的线程不是同一线程,
    // 那么T必须继承自RefCountedThreadSafe<T>!
    template<class T>
    void ReleaseSoon(T* object)
    {
        PostNonNestableTask(new ReleaseTask<T>(object));
    }

    // 执行消息循环.
    void Run();

    // 处理所有等待的任务、Windows消息等, 不会等待/休眠. 处理好所有可执行的条目
    // 之后立即返回.
    void RunAllPending();

    // 通知Run方法在所有等待消息处理完成之后返回, 只能在调用Run的相同线程中使用,
    // 且Run必须还在执行.
    //
    // 如果需要停止另一个线程的MessageLoop, 可以使用QuitTask, 但是需要注意, 如果
    // 目标线程嵌套调用了MessageLoop::Run, 这么做相当危险.
    void Quit();

    // Quit的另一种形式, 不会等待挂起的消息被处理, Run直接返回.
    void QuitNow();

    // 当前运行的MessageLoop中调用Quit. 调度任意MessageLoop的Quit方法.
    class QuitTask : public Task
    {
    public:
        virtual void Run()
        {
            MessageLoop::current()->Quit();
        }
    };

    // 返回传递给构造函数的消息循环类型.
    Type type() const { return type_; }

    // 把线程的名字和消息循环关联上(选择性调用).
    void set_thread_name(const std::string& thread_name)
    {
        DCHECK(thread_name_.empty()) << "Should not rename this thread!";
        thread_name_ = thread_name;
    }
    const std::string& thread_name() const { return thread_name_; }

    // Gets the message loop proxy associated with this message loop proxy
    scoped_refptr<base::MessageLoopProxy> message_loop_proxy()
    {
        return message_loop_proxy_.get();
    }

    // 允许或者禁止处理递归的任务. 在递归的消息循环中发生. 当使用通用控件或者
    // 打印函数的时候, 会出现一些讨厌的消息循环. 缺省情况下禁止处理递归的任务.
    //
    // 
    // 任务排队的特定情况:
    // - 线程正在执行消息循环.
    // - 接收任务#1并执行.
    // - 任务#1也启动了一个消息循环, 类似MessageBox、StartDoc或者GetSaveFileName.
    // - 在第二个消息循环之前或者执行中, 线程接收到任务#2.
    // - 当NestableTasksAllowed设置为true, 任务#2会立即执行. 否则, 将在任务#1完成
    //   消息循环之后才会被执行.
    void SetNestableTasksAllowed(bool allowed);
    bool NestableTasksAllowed() const;

    // 在一个作用域内允许|loop|执行嵌套任务.
    class ScopedNestableTaskAllower
    {
    public:
        explicit ScopedNestableTaskAllower(MessageLoop* loop)
            : loop_(loop), old_state_(loop_->NestableTasksAllowed())
        {
            loop_->SetNestableTasksAllowed(true);
        }
        ~ScopedNestableTaskAllower()
        {
            loop_->SetNestableTasksAllowed(old_state_);
        }

    private:
        MessageLoop* loop_;
        bool old_state_;
    };

    // 允许或者禁止使用调用Run()前设置的未处理异常过滤器处理异常. 如果第三方
    // 代码调用了SetUnhandledExceptionFilter()但是从没恢复, 会出现这种情况.
    void set_exception_restoration(bool restore)
    {
        exception_restoration_ = restore;
    }

    // 如果当前运行在嵌套的消息循环返回true.
    bool IsNested();

    // TaskObserver对象从MessageLoop接收任务通知.
    //
    // 注意: TaskObserver的实现应该是非常快的!
    class TaskObserver
    {
    public:
        TaskObserver();

        // 处理任务前调用.
        virtual void WillProcessTask(base::TimeTicks time_posted) = 0;

        // 处理任务后调用.
        virtual void DidProcessTask(base::TimeTicks time_posted) = 0;

    protected:
        virtual ~TaskObserver();
    };

    // 只能在当前消息循环运行的线程中调用.
    void AddTaskObserver(TaskObserver* task_observer);
    void RemoveTaskObserver(TaskObserver* task_observer);

    // 如果消息循环启用了高精度时钟返回true. 用于测试.
    bool high_resolution_timers_enabled()
    {
        return !high_resolution_timer_expiration_.is_null();
    }

    // 启用高精度时钟模式时, 至少维持1s.
    static const int kHighResolutionTimerModeLeaseTimeMs = 1000;

    // 断言MessageLoop是"空闲的".
    void AssertIdle() const;

    void set_os_modal_loop(bool os_modal_loop)
    {
        os_modal_loop_ = os_modal_loop;
    }

    bool os_modal_loop() const
    {
        return os_modal_loop_;
    }

protected:
    struct RunState
    {
        // Run()调用栈计数.
        int run_depth;

        // 记录是否调用了Quit(), 一旦消息泵空闲, 循环将会停止.
        bool quit_received;

        Dispatcher* dispatcher;
    };

    class AutoRunState : RunState
    {
    public:
        explicit AutoRunState(MessageLoop* loop);
        ~AutoRunState();

    private:
        MessageLoop* loop_;
        RunState* previous_state_;
    };

    // 结构体按值拷贝.
    struct PendingTask
    {
        PendingTask(const base::Closure& task,
            base::TimeTicks delayed_run_time,
            bool nestable);
        ~PendingTask();

        // 用于支持排序.
        bool operator<(const PendingTask& other) const;

        // The task to run.
        base::Closure task;

        // Time this PendingTask was posted.
        base::TimeTicks time_posted;

        // The time when the task should be run.
        base::TimeTicks delayed_run_time;

        // Secondary sort key for run time.
        int sequence_num;

        // OK to dispatch from a nested loop.
        bool nestable;
    };

    class TaskQueue : public std::queue<PendingTask>
    {
    public:
        void Swap(TaskQueue* queue)
        {
            c.swap(queue->c); // 调用std::deque::swap
        }
    };

    typedef std::priority_queue<PendingTask> DelayedTaskQueue;

    base::MessagePumpWin* pump_win()
    {
        return static_cast<base::MessagePumpWin*>(pump_.get());
    }

    // 函数封装对主消息循环执行堆栈中所有异常的处理能力. 是否在SEH的try块中
    // 执行消息循环依赖于set_exception_restoration()设置的标志位, 分别调用
    // RunInternalInSEHFrame()或RunInternal().
    void RunHandler();

    __declspec(noinline) void RunInternalInSEHFrame();

    // 在消息循环执行的周围添加一些栈信息, 支持状态的保存和恢复用于递归调用.
    void RunInternal();

    // 调用处理所有延迟的非嵌套任务.
    bool ProcessNextDelayedNonNestableTask();

    // 执行特定的任务.
    void RunTask(const PendingTask& pending_task);

    // 调用RunTask, 如果不能立即执行则添加pending_task到延迟队列. 任务被执行返
    // 回true.
    bool DeferOrRunPendingTask(const PendingTask& pending_task);

    // 添加pending_task到delayed_work_queue_.
    void AddToDelayedWorkQueue(const PendingTask& pending_task);

    // Adds the pending task to our incoming_queue_.
    //
    // Caller retains ownership of |pending_task|, but this function will
    // reset the value of pending_task->task.  This is needed to ensure
    // that the posting call stack does not retain pending_task->task
    // beyond this function call.
    void AddToIncomingQueue(PendingTask* pending_task);

    // 如果work_queue_为空, 从incoming_queue_加载任务到work_queue_. 访问
    // incoming_queue_需要加锁, work_queue_可在本线程直接访问.
    void ReloadWorkQueue();

    // 删除还没有运行的任务, 这些任务不被执行. 用在析构函数中确保所有的任务
    // 都能被析构. 如果存在这样的任务, 返回true.
    bool DeletePendingTasks();

    // Calcuates the time at which a PendingTask should run.
    base::TimeTicks CalculateDelayedRuntime(int64 delay_ms);

    // Start recording histogram info about events and action IF it was enabled
    // and IF the statistics recorder can accept a registration of our histogram.
    void StartHistogrammer();

    // Add occurence of event to our histogram, so that we can see what is being
    // done in a specific MessageLoop instance (i.e., specific thread).
    // If message_histogram_ is NULL, this is a no-op.
    void HistogramEvent(int event);

    // base::MessagePump::Delegate methods:
    virtual bool DoWork();
    virtual bool DoDelayedWork(base::TimeTicks* next_delayed_work_time);
    virtual bool DoIdleWork();

    Type type_;

    // 需要立即被处理的任务列表. 注意队列只能在当前线程访问(push/pop).
    TaskQueue work_queue_;

    // 存储延迟任务, 按照'delayed_run_time'属性排序.
    DelayedTaskQueue delayed_work_queue_;

    // 最近一次调用Time::Now()的快照, 用于检查delayed_work_queue_.
    base::TimeTicks recent_time_;

    // 在嵌套的消息循环中执行非嵌套的任务, 这些任务必须排队以延迟执行. 当离开
    // 嵌套消息循环, 它们立马被执行.
    TaskQueue deferred_non_nestable_work_queue_;

    scoped_refptr<base::MessagePump> pump_;

    ObserverList<DestructionObserver> destruction_observers_;

    // 在一个嵌套的消息泵中阻止任务执行.
    bool nestable_tasks_allowed_;

    bool exception_restoration_;

    std::string thread_name_;

    // A profiling histogram showing the counts of various messages and events.
    base::Histogram* message_histogram_;

    // 用于接收到来的任务, 这些任务还未被交换到work_queue_.
    TaskQueue incoming_queue_;
    // incoming_queue_访问保护.
    mutable base::Lock incoming_queue_lock_;

    RunState* state_;

    // The need for this variable is subtle. Please see implementation comments
    // around where it is used.
    bool should_leak_tasks_;

    base::TimeTicks high_resolution_timer_expiration_;
    // 调用TrackPopupMenu这样的Windows API时候设置为true, 进入模态消息循环.
    bool os_modal_loop_;

    // 延迟任务使用的下一个序号.
    int next_sequence_num_;

    ObserverList<TaskObserver> task_observers_;

    // The message loop proxy associated with this message loop, if one exists.
    scoped_refptr<base::MessageLoopProxy> message_loop_proxy_;

private:
    DISALLOW_COPY_AND_ASSIGN(MessageLoop);
};

//-----------------------------------------------------------------------------
// MessageLoopForUI扩展MessageLoop, 用TYPE_UI初始化MessageLoop创建UI消息泵.
//
// 该类的常见用法:
//     MessageLoopForUI::current()->...调用某个方法...
class MessageLoopForUI : public MessageLoop
{
public:
    MessageLoopForUI() : MessageLoop(TYPE_UI) {}

    // 返回当前线程的MessageLoopForUI.
    static MessageLoopForUI* current()
    {
        MessageLoop* loop = MessageLoop::current();
        DCHECK_EQ(MessageLoop::TYPE_UI, loop->type());
        return static_cast<MessageLoopForUI*>(loop);
    }

    void DidProcessMessage(const MSG& message);

    void AddObserver(Observer* observer);
    void RemoveObserver(Observer* observer);
    void Run(Dispatcher* dispatcher);

protected:
    base::MessagePumpForUI* pump_ui()
    {
        return static_cast<base::MessagePumpForUI*>(pump_.get());
    }
};
// 不要给MessageLoopForUI添加任何成员! 因为MessageLoopForUI的分配一般是通过
// MessageLoop(TYPE_UI), 任何额外的数据都应该存储到MessageLoop的pump_实例中.
COMPILE_ASSERT(sizeof(MessageLoop)==sizeof(MessageLoopForUI),
               MessageLoopForUI_should_not_have_extra_member_variables);

//-----------------------------------------------------------------------------
// MessageLoopForUI扩展MessageLoop, 用TYPE_IO初始化MessageLoop创建IO消息泵.
//
// 该类的常见用法:
//     MessageLoopForIO::current()->...调用某个方法...
class MessageLoopForIO : public MessageLoop
{
public:
    typedef base::MessagePumpForIO::IOHandler IOHandler;
    typedef base::MessagePumpForIO::IOContext IOContext;
    typedef base::MessagePumpForIO::IOObserver IOObserver;

    MessageLoopForIO() : MessageLoop(TYPE_IO) {}

    // 返回当前线程的MessageLoopForIO.
    static MessageLoopForIO* current()
    {
        MessageLoop* loop = MessageLoop::current();
        DCHECK_EQ(MessageLoop::TYPE_IO, loop->type());
        return static_cast<MessageLoopForIO*>(loop);
    }

    void AddIOObserver(IOObserver* io_observer)
    {
        pump_io()->AddIOObserver(io_observer);
    }

    void RemoveIOObserver(IOObserver* io_observer)
    {
        pump_io()->RemoveIOObserver(io_observer);
    }

    void RegisterIOHandler(HANDLE file_handle, IOHandler* handler);
    bool WaitForIOCompletion(DWORD timeout, IOHandler* filter);

protected:
    base::MessagePumpForIO* pump_io()
    {
        return static_cast<base::MessagePumpForIO*>(pump_.get());
    }
};
// 不要给MessageLoopForIO添加任何成员! 因为MessageLoopForIO的分配一般是通过
// MessageLoop(TYPE_IO), 任何额外的数据都应该存储到MessageLoop的pump_实例中.
COMPILE_ASSERT(sizeof(MessageLoop)==sizeof(MessageLoopForIO),
               MessageLoopForIO_should_not_have_extra_member_variables);

#endif //__base_message_loop_h__