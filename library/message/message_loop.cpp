
#include "message_loop.h"

#include "base/lazy_instance.h"
#include "base/metric/histogram.h"
#include "base/threading/thread_local.h"

#include "message_pump_default.h"

namespace
{

    // 懒创建(lazily created)的TLS, 用于快速访问线程的消息循环(如果存在).
    // 相当于一个安全、方便的静态构造函数.
    base::LazyInstance<base::ThreadLocalPointer<MessageLoop> > lazy_tls_ptr(
        base::LINKER_INITIALIZED);

    // Logical events for Histogram profiling. Run with -message-loop-histogrammer
    // to get an accounting of messages and actions taken on each thread.
    const int kTaskRunEvent = 0x1;
    const int kTimerEvent = 0x2;

    // Provide range of message IDs for use in histogramming and debug display.
    const int kLeastNonZeroMessageId = 1;
    const int kMaxMessageId = 1099;
    const int kNumberOfDistinctMessagesDisplayed = 1100;

    // Provide a macro that takes an expression (such as a constant, or macro
    // constant) and creates a pair to initalize an array of pairs.  In this case,
    // our pair consists of the expressions value, and the "stringized" version
    // of the expression (i.e., the exrpression put in quotes).  For example, if
    // we have:
    //    #define FOO 2
    //    #define BAR 5
    // then the following:
    //    VALUE_TO_NUMBER_AND_NAME(FOO + BAR)
    // will expand to:
    //   {7, "FOO + BAR"}
    // We use the resulting array as an argument to our histogram, which reads the
    // number as a bucket identifier, and proceeds to use the corresponding name
    // in the pair (i.e., the quoted string) when printing out a histogram.
#define VALUE_TO_NUMBER_AND_NAME(name) { name, #name },

    const base::LinearHistogram::DescriptionPair event_descriptions_[] =
    {
        // Provide some pretty print capability in our histogram for our internal
        // messages.

        // A few events we handle (kindred to messages), and used to profile actions.
        VALUE_TO_NUMBER_AND_NAME(kTaskRunEvent)
        VALUE_TO_NUMBER_AND_NAME(kTimerEvent)

        { -1, NULL } // The list must be null terminated, per API to histogram.
    };

    bool enable_histogrammer_ = false;

}

// 在线程遇到SEH异常时, 恢复旧的未处理异常过滤器.
static int SEHFilter(LPTOP_LEVEL_EXCEPTION_FILTER old_filter)
{
    ::SetUnhandledExceptionFilter(old_filter);
    return EXCEPTION_CONTINUE_SEARCH;
}

// 返回当前的未处理异常过滤器指针, 没有可直接获取的API.
static LPTOP_LEVEL_EXCEPTION_FILTER GetTopSEHFilter()
{
    LPTOP_LEVEL_EXCEPTION_FILTER top_filter = NULL;
    top_filter = ::SetUnhandledExceptionFilter(0);
    ::SetUnhandledExceptionFilter(top_filter);
    return top_filter;
}

MessageLoop::TaskObserver::TaskObserver() {}

MessageLoop::TaskObserver::~TaskObserver() {}

MessageLoop::DestructionObserver::~DestructionObserver() {}

MessageLoop::MessageLoop(Type type)
: type_(type),
nestable_tasks_allowed_(true),
exception_restoration_(false),
state_(NULL),
os_modal_loop_(false),
next_sequence_num_(0)
{
    DCHECK(!current()) << "should only have one message loop per thread";
    lazy_tls_ptr.Pointer()->Set(this);

#define MESSAGE_PUMP_UI new base::MessagePumpForUI()
#define MESSAGE_PUMP_IO new base::MessagePumpForIO()

    if(type_ == TYPE_UI)
    {
        pump_ = MESSAGE_PUMP_UI;
    }
    else if(type_ == TYPE_IO)
    {
        pump_ = MESSAGE_PUMP_IO;
    }
    else
    {
        DCHECK_EQ(TYPE_DEFAULT, type_);
        pump_ = new base::MessagePumpDefault();
    }
}

MessageLoop::~MessageLoop()
{
    DCHECK(this == current());

    DCHECK(!state_);

    // 清理所有未处理的任务, 但是注意: 删除一个任务可能会导致添加新的任务(比如通过
    // DeleteSoon). 我们对被删除任务生成新任务的次数设置一个最大限制. 一般来说, 循
    // 环执行一两次就会退出. 如果结束时遇到循环的最大限制值, 可能是有一个任务一直在
    // 那里缠绕着, 可以检查队列看看留下的是什么任务.
    bool did_work;
    for(int i=0; i<100; ++i)
    {
        DeletePendingTasks();
        ReloadWorkQueue();
        // 如果队列为空, 跳出循环.
        did_work = DeletePendingTasks();
        if(!did_work)
        {
            break;
        }
    }
    DCHECK(!did_work);

    // 给DestructionObservers最后访问this的机会.
    FOR_EACH_OBSERVER(DestructionObserver, destruction_observers_,
        WillDestroyCurrentMessageLoop());

    // 设置为NULL, 对象不会再被访问到.
    lazy_tls_ptr.Pointer()->Set(NULL);
}

// static
MessageLoop* MessageLoop::current()
{
    return lazy_tls_ptr.Pointer()->Get();
}

void MessageLoop::AddDestructionObserver(DestructionObserver* destruction_observer)
{
    DCHECK(this == current());
    destruction_observers_.AddObserver(destruction_observer);
}

void MessageLoop::RemoveDestructionObserver(
    DestructionObserver* destruction_observer)
{
    DCHECK(this == current());
    destruction_observers_.RemoveObserver(destruction_observer);
}

void MessageLoop::PostTask(Task* task)
{
    PostTask_Helper(task, 0, true);
}

void MessageLoop::PostDelayedTask(Task* task, int64 delay_ms)
{
    PostTask_Helper(task, delay_ms, true);
}

void MessageLoop::PostNonNestableTask(Task* task)
{
    PostTask_Helper(task, 0, false);
}

void MessageLoop::PostNonNestableDelayedTask(Task* task, int64 delay_ms)
{
    PostTask_Helper(task, delay_ms, false);
}

void MessageLoop::Run()
{
    AutoRunState save_state(this);
    RunHandler();
}

void MessageLoop::RunAllPending()
{
    AutoRunState save_state(this);
    state_->quit_received = true; // 运行直到空闲时退出, 否则会进入等待状态.
    RunHandler();
}

void MessageLoop::Quit()
{
    DCHECK(current() == this);
    if(state_)
    {
        state_->quit_received = true;
    }
    else
    {
        NOTREACHED() << "Must be inside Run to call Quit";
    }
}

void MessageLoop::QuitNow()
{
    DCHECK(current() == this);
    if(state_)
    {
        pump_->Quit();
    }
    else
    {
        NOTREACHED() << "Must be inside Run to call Quit";
    }
}

void MessageLoop::SetNestableTasksAllowed(bool allowed)
{
    if(nestable_tasks_allowed_ != allowed)
    {
        nestable_tasks_allowed_ = allowed;
        if(!nestable_tasks_allowed_)
        {
            return;
        }
        // 启动本地消息泵.
        pump_->ScheduleWork();
    }
}

bool MessageLoop::NestableTasksAllowed() const
{
    return nestable_tasks_allowed_;
}

bool MessageLoop::IsNested()
{
    return state_->run_depth > 1;
}

void MessageLoop::AddTaskObserver(TaskObserver* task_observer)
{
    DCHECK_EQ(this, current());
    task_observers_.AddObserver(task_observer);
}

void MessageLoop::RemoveTaskObserver(TaskObserver* task_observer)
{
    DCHECK_EQ(this, current());
    task_observers_.RemoveObserver(task_observer);
}

void MessageLoop::AssertIdle() const
{
    // 只检查|incoming_queue_|, 因为不想对|work_queue_|加锁.
    base::AutoLock lock(incoming_queue_lock_);
    DCHECK(incoming_queue_.empty());
}

// 在两种SEH模式下执行循环:
// exception_restoration_ = false: 所有未处理异常都交给最后调用
// SetUnhandledExceptionFilter()设置的过滤器.
// exception_restoration_ = true: 所有未处理异常都交给循环执行前的过滤器.
void MessageLoop::RunHandler()
{
    if(exception_restoration_)
    {
        RunInternalInSEHFrame();
        return;
    }

    RunInternal();
}

__declspec(noinline) void MessageLoop::RunInternalInSEHFrame()
{
    LPTOP_LEVEL_EXCEPTION_FILTER current_filter = GetTopSEHFilter();
    __try
    {
        RunInternal();
    }
    __except(SEHFilter(current_filter))
    {
    }
    return;
}

void MessageLoop::RunInternal()
{
    DCHECK(this == current());

    if(state_->dispatcher && type()==TYPE_UI)
    {
        static_cast<base::MessagePumpForUI*>(pump_.get())->
            RunWithDispatcher(this, state_->dispatcher);
        return;
    }

    pump_->Run(this);
}

bool MessageLoop::ProcessNextDelayedNonNestableTask()
{
    if(state_->run_depth != 1)
    {
        return false;
    }

    if(deferred_non_nestable_work_queue_.empty())
    {
        return false;
    }

    Task* task = deferred_non_nestable_work_queue_.front().task;
    deferred_non_nestable_work_queue_.pop();

    RunTask(task);
    return true;
}

void MessageLoop::RunTask(Task* task)
{
    DCHECK(nestable_tasks_allowed_);
    // 执行任务, 采取最严格方式: 不能重入.
    nestable_tasks_allowed_ = false;

    HistogramEvent(kTaskRunEvent);
    FOR_EACH_OBSERVER(TaskObserver, task_observers_, WillProcessTask(task));
    task->Run();
    FOR_EACH_OBSERVER(TaskObserver, task_observers_, DidProcessTask(task));
    delete task;

    nestable_tasks_allowed_ = true;
}

bool MessageLoop::DeferOrRunPendingTask(const PendingTask& pending_task)
{
    if(pending_task.nestable || state_->run_depth==1)
    {
        RunTask(pending_task.task);
        // 表明执行力一个任务(注意: 随后可能会到达一个新任务!).
        return true;
    }

    // 现在不能执行任务, 因为在嵌套消息循环中且任务不允许嵌套执行.
    deferred_non_nestable_work_queue_.push(pending_task);
    return false;
}

void MessageLoop::AddToDelayedWorkQueue(const PendingTask& pending_task)
{
    // 移到延迟任务队列. 插入delayed_work_queue_队列前初始化序号. 序号在对
    // 相同延迟执行时间的两个任务排序时会用到, 保证先进先出.
    PendingTask new_pending_task(pending_task);
    new_pending_task.sequence_num = next_sequence_num_++;
    delayed_work_queue_.push(new_pending_task);
}

void MessageLoop::ReloadWorkQueue()
{
    // 当work_queue_为空的时候才从incoming_queue_加载任务, 这样可以减少对
    // incoming_queue_加锁次数, 提高性能.
    if(!work_queue_.empty())
    {
        return; // 有必要时, 才对incoming_queue_lock_加锁并从中加载任务.
    }

    // 上锁避免线程冲突.
    {
        base::AutoLock lock(incoming_queue_lock_);
        if(incoming_queue_.empty())
        {
            return;
        }
        incoming_queue_.Swap(&work_queue_); // 常量时间.
        DCHECK(incoming_queue_.empty());
    }
}

bool MessageLoop::DeletePendingTasks()
{
    bool did_work = !work_queue_.empty();
    while(!work_queue_.empty())
    {
        PendingTask pending_task = work_queue_.front();
        work_queue_.pop();
        if(!pending_task.delayed_run_time.is_null())
        {
            // 延迟任务的删除顺序和正常删除顺序保持一致. 延迟任务之间可能存在依赖关系.
            AddToDelayedWorkQueue(pending_task);
        }
        else
        {
            delete pending_task.task;
        }
    }
    did_work |= !deferred_non_nestable_work_queue_.empty();
    while(!deferred_non_nestable_work_queue_.empty())
    {
        Task* task = NULL;
        task = deferred_non_nestable_work_queue_.front().task;
        deferred_non_nestable_work_queue_.pop();
        if(task)
        {
            delete task;
        }
    }
    did_work |= !delayed_work_queue_.empty();
    while(!delayed_work_queue_.empty())
    {
        Task* task = delayed_work_queue_.top().task;
        delayed_work_queue_.pop();
        delete task;
    }
    return did_work;
}

// 可能被后台线程调用!
void MessageLoop::PostTask_Helper(Task* task, int64 delay_ms, bool nestable)
{
    PendingTask pending_task(task, nestable);

    if(delay_ms > 0)
    {
        pending_task.delayed_run_time =
            base::TimeTicks::Now() + base::TimeDelta::FromMilliseconds(delay_ms);

        if(high_resolution_timer_expiration_.is_null())
        {
            // Windows时钟的精度是15.6ms. 如果只对小于15.6ms的时钟启用高精度时
            // 钟, 那么18ms的时钟大约会在32ms时触发, 精度误差非常大. 因此需要
            // 对所有小于2倍15.6ms的时钟启用高精度时钟. 这是精度和电源管理之间
            // 的平衡.
            bool needs_high_res_timers =
                delay_ms < (2 * base::Time::kMinLowResolutionThresholdMs);
            if(needs_high_res_timers)
            {
                base::Time::ActivateHighResolutionTimer(true);
                high_resolution_timer_expiration_ = base::TimeTicks::Now() +
                    base::TimeDelta::FromMilliseconds(kHighResolutionTimerModeLeaseTimeMs);
            }
        }
    }
    else
    {
        DCHECK_EQ(delay_ms, 0) << "delay should not be negative";
    }

    if(!high_resolution_timer_expiration_.is_null())
    {
        if(base::TimeTicks::Now() > high_resolution_timer_expiration_)
        {
            base::Time::ActivateHighResolutionTimer(false);
            high_resolution_timer_expiration_ = base::TimeTicks();
        }
    }

    // 警告: 不要在当前线程中直接循环执行任务, 这样可能会堵塞外部线程.
    // 把所有的任务都放入队列.

    scoped_refptr<base::MessagePump> pump;
    {
        base::AutoLock locked(incoming_queue_lock_);

        bool was_empty = incoming_queue_.empty();
        incoming_queue_.push(pending_task);
        if(!was_empty)
        {
            return; // 在别处启动了子消息泵.
        }

        pump = pump_;
    }
    // 因为incoming_queue_中可能有销毁本消息循环的任务, 所以函数完成之前不能
    // 解除锁保护. 使用一个栈对象引用消息泵, 这样我们可以在incoming_queue_lock_
    // 锁外面调用ScheduleWork.

    pump->ScheduleWork();
}

//------------------------------------------------------------------------------
// Method and data for histogramming events and actions taken by each instance
// on each thread.

void MessageLoop::StartHistogrammer()
{
    if(enable_histogrammer_ && !message_histogram_
        && base::StatisticsRecorder::IsActive())
    {
        DCHECK(!thread_name_.empty());
        message_histogram_ = base::LinearHistogram::FactoryGet(
            "MsgLoop:"+thread_name_,
            kLeastNonZeroMessageId, kMaxMessageId,
            kNumberOfDistinctMessagesDisplayed,
            message_histogram_->kHexRangePrintingFlag);
        message_histogram_->SetRangeDescriptions(event_descriptions_);
    }
}

void MessageLoop::HistogramEvent(int event)
{
    if(message_histogram_)
    {
        message_histogram_->Add(event);
    }
}

bool MessageLoop::DoWork()
{
    if(!nestable_tasks_allowed_)
    {
        // 任务不能立即被执行.
        return false;
    }

    for(;;)
    {
        ReloadWorkQueue();
        if(work_queue_.empty())
        {
            break;
        }

        // 执行最早的任务.
        do
        {
            PendingTask pending_task = work_queue_.front();
            work_queue_.pop();
            if(!pending_task.delayed_run_time.is_null())
            {
                AddToDelayedWorkQueue(pending_task);
                // 如果最上面的任务发生变化, 需要重新调度时间.
                if(delayed_work_queue_.top().task == pending_task.task)
                {
                    pump_->ScheduleDelayedWork(pending_task.delayed_run_time);
                }
            }
            else
            {
                if(DeferOrRunPendingTask(pending_task))
                {
                    return true;
                }
            }
        } while(!work_queue_.empty());
    }

    // 什么都没发生.
    return false;
}

bool MessageLoop::DoDelayedWork(base::TimeTicks* next_delayed_work_time)
{
    if(!nestable_tasks_allowed_ || delayed_work_queue_.empty())
    {
        recent_time_ = *next_delayed_work_time = base::TimeTicks();
        return false;
    }

    // 当滞后的时候, 说明有大量的任务在延迟队列等待运行. 为了提高效率, 只是不断
    // 地调用Time::Now(), 以处理所有等待运行的任务. 这样越是滞后(有大量等待运行
    // 的任务), 任务处理能力越高效.

    base::TimeTicks next_run_time = delayed_work_queue_.top().delayed_run_time;
    if(next_run_time > recent_time_)
    {
        recent_time_ = base::TimeTicks::Now(); // 比Now()更清晰;
        if(next_run_time > recent_time_)
        {
            *next_delayed_work_time = next_run_time;
            return false;
        }
    }

    PendingTask pending_task = delayed_work_queue_.top();
    delayed_work_queue_.pop();

    if(!delayed_work_queue_.empty())
    {
        *next_delayed_work_time = delayed_work_queue_.top().delayed_run_time;
    }

    return DeferOrRunPendingTask(pending_task);
}

bool MessageLoop::DoIdleWork()
{
    if(ProcessNextDelayedNonNestableTask())
    {
        return true;
    }

    if(state_->quit_received)
    {
        pump_->Quit();
    }

    return false;
}

MessageLoop::AutoRunState::AutoRunState(MessageLoop* loop) : loop_(loop)
{
    previous_state_ = loop_->state_;
    if(previous_state_)
    {
        run_depth = previous_state_->run_depth + 1;
    }
    else
    {
        run_depth = 1;
    }
    loop_->state_ = this;

    quit_received = false;
    dispatcher = NULL;
}

MessageLoop::AutoRunState::~AutoRunState()
{
    loop_->state_ = previous_state_;
}

bool MessageLoop::PendingTask::operator<(const PendingTask& other) const
{
    // 因为优先级队列最上面的元素优先级最大, 这里的比较需要反过来. 时间最小的
    // 在最上面.
    if(delayed_run_time < other.delayed_run_time)
    {
        return false;
    }

    if(delayed_run_time > other.delayed_run_time)
    {
        return true;
    }

    // 如果时间刚好相等, 通过序号决定. 比对序号的差值, 这样可以支持整数的溢出.
    return (sequence_num - other.sequence_num) > 0;
}


void MessageLoopForUI::DidProcessMessage(const MSG& message)
{
    pump_win()->DidProcessMessage(message);
}

void MessageLoopForUI::AddObserver(Observer* observer)
{
    pump_ui()->AddObserver(observer);
}

void MessageLoopForUI::RemoveObserver(Observer* observer)
{
    pump_ui()->RemoveObserver(observer);
}

void MessageLoopForUI::Run(Dispatcher* dispatcher)
{
    AutoRunState save_state(this);
    state_->dispatcher = dispatcher;
    RunHandler();
}


void MessageLoopForIO::RegisterIOHandler(HANDLE file, IOHandler* handler)
{
    pump_io()->RegisterIOHandler(file, handler);
}

bool MessageLoopForIO::WaitForIOCompletion(DWORD timeout, IOHandler* filter)
{
    return pump_io()->WaitForIOCompletion(timeout, filter);
}