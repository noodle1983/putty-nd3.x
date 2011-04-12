
#ifndef __message_timer_h__
#define __message_timer_h__

#pragma once

#include "base/time.h"

#include "task.h"

// OneShotTimer和RepeatingTimer提供了方便的时钟API. 从命名可以看出,
// OneShotTimer在给定的时间延迟过期时会回调, RepeatingTimer会定期的
// 回调.
// 
// OneShotTimer和RepeatingTimer都会在离开作用域的时候取消时钟, 这样
// 很容易保证对象离开作用域之后不会再回调. 如果类想接受时钟时间, 只
// 需要实例化一个OneShotTimer或者RepeatingTimer成员变量.
//
// RepeatingTimer用法示例:
//
//     class MyClass {
//     public:
//       void StartDoingStuff() {
//         timer_.Start(TimeDelta::FromSeconds(1), this, &MyClass::DoStuff);
//       }
//       void StopDoingStuff() {
//         timer_.Stop();
//       }
//     private:
//       void DoStuff() {
//         // This method is called every second to do stuff.
//         ...
//       }
//       base::RepeatingTimer<MyClass> timer_;
//     };
//
// OneShotTimer和RepeatingTimer都支持Reset方法, 你可以很容易的延迟时钟事件
// 到下一个延迟时间触发. 所以在上面的例子中, 0.5秒的时候调用timer_的Reset,
// 那么DoStuff的调用会延迟到下一秒. 换句话说, Reset等同于先调用Stop, 然后
// 用一样的参数调用Start.

class MessageLoop;

namespace base
{

    //-------------------------------------------------------------------------
    // BaseTimer_Helper是OneShotTimer和RepeatingTimer的内部实现. 不要直接使用.
    //
    // BaseTimer<T>模板实例之间共享的代码.
    class BaseTimer_Helper
    {
    public:
        // 停止时钟.
        ~BaseTimer_Helper()
        {
            OrphanDelayedTask();
        }

        // 返回时钟是否正在运行.
        bool IsRunning() const
        {
            return delayed_task_ != NULL;
        }

        // 返回时钟当前的延迟时间. 只能在时钟运行时调用!
        TimeDelta GetCurrentDelay() const
        {
            DCHECK(IsRunning());
            return delayed_task_->delay_;
        }

    protected:
        BaseTimer_Helper() : delayed_task_(NULL) {}

        // 可以访问timer_的成员, 这样我们可以把任务跟时钟分离开.
        class TimerTask : public Task
        {
        public:
            explicit TimerTask(TimeDelta delay) : timer_(NULL), delay_(delay) {}
            virtual ~TimerTask() {}
            BaseTimer_Helper* timer_;
            TimeDelta delay_;
        };

        // 把delayed_task_跟时钟分离开, 这样当它执行时什么都不做.
        void OrphanDelayedTask();

        // 初始化一个新的延迟任务. 如果delayed_task_不为空, 会先把它跟时钟分离.
        void InitiateDelayedTask(TimerTask* timer_task);

        TimerTask* delayed_task_;

        DISALLOW_COPY_AND_ASSIGN(BaseTimer_Helper);
    };

    //-------------------------------------------------------------------------
    // BaseTimer是OneShotTimer和RepeatingTimer的内部实现. 不要直接使用.
    template<class Receiver, bool kIsRepeating>
    class BaseTimer : public BaseTimer_Helper
    {
    public:
        typedef void (Receiver::*ReceiverMethod)();

        // 启动时钟. 不要在时钟运行的时候调用.
        void Start(TimeDelta delay, Receiver* receiver, ReceiverMethod method)
        {
            DCHECK(!IsRunning());
            InitiateDelayedTask(new TimerTask(delay, receiver, method));
        }

        // 停止时钟. 如果时钟没有运行, 不做任何事情.
        void Stop()
        {
            OrphanDelayedTask();
        }

        // 重置正在运行的时钟的等待时间.
        void Reset()
        {
            DCHECK(IsRunning());
            InitiateDelayedTask(static_cast<TimerTask*>(delayed_task_)->Clone());
        }

    private:
        typedef BaseTimer<Receiver, kIsRepeating> SelfType;

        class TimerTask : public BaseTimer_Helper::TimerTask
        {
        public:
            TimerTask(TimeDelta delay, Receiver* receiver, ReceiverMethod method)
                : BaseTimer_Helper::TimerTask(delay),
                receiver_(receiver), method_(method) {}

            virtual ~TimerTask()
            {
                // 任务正在清理有可能是因为MessageLoop已经被析构. 如果是这样, 不
                // 要给正在销毁的任务留下一个Timer的野指针.
                ClearBaseTimer();
            }

            virtual void Run()
            {
                if(!timer_) // 如果被孤立, timer_为空.
                {
                    return;
                }
                if(kIsRepeating)
                {
                    ResetBaseTimer();
                }
                else
                {
                    ClearBaseTimer();
                }
                DispatchToMethod(receiver_, method_, Tuple0());
            }

            TimerTask* Clone() const
            {
                return new TimerTask(delay_, receiver_, method_);
            }

        private:
            // 通知基类时钟已经停止.
            void ClearBaseTimer()
            {
                if(timer_)
                {
                    SelfType* self = static_cast<SelfType*>(timer_);
                    // 时钟有可能已经被重置, 这是一个旧的任务. 所以, 如果Timer指向
                    // 了一个不同的任务, 可以断定Timer已经重新设置了任务.
                    if(self->delayed_task_ == this)
                    {
                        self->delayed_task_ = NULL;
                    }
                    // 此时Timer中的delayed_task_不再指向我们. 需要自己重置timer_,
                    // 因为Timer在它的析构函数中没做.
                    timer_ = NULL;
                }
            }

            // 通知基类正在重置时钟.
            void ResetBaseTimer()
            {
                DCHECK(timer_);
                DCHECK(kIsRepeating);
                SelfType* self = static_cast<SelfType*>(timer_);
                self->Reset();
            }

            Receiver* receiver_;
            ReceiverMethod method_;
        };
    };

    //-------------------------------------------------------------------------
    // 简单的一次调用时钟.
    template<class Receiver>
    class OneShotTimer : public BaseTimer<Receiver, false> {};

    //-------------------------------------------------------------------------
    // 简单的重复调用时钟.
    template<class Receiver>
    class RepeatingTimer : public BaseTimer<Receiver, true> {};

    //-------------------------------------------------------------------------
    // 延迟时钟类似弹起的按钮. 启动后, 必须调用Reset, 才会在MessageLoop的线程中执
    // 行给定的方法.
    //
    // 对象创建后, 只有调用Reset才会启动时钟. 自上次调用Reset, 经过|delay|秒后执行
    // 回调. 回调完成后, 只有再次调用Reset才能重新启动时钟.
    //
    // 删除对象, 超时被取消, 运行的时钟不会被执行.
    template<class Receiver>
    class DelayTimer
    {
    public:
        typedef void (Receiver::*ReceiverMethod)();

        DelayTimer(TimeDelta delay, Receiver* receiver, ReceiverMethod method)
            : receiver_(receiver), method_(method), delay_(delay) {}

        void Reset()
        {
            DelayFor(delay_);
        }

    private:
        void DelayFor(TimeDelta delay)
        {
            trigger_time_ = Time::Now() + delay;

            // 如果已经有一个时钟, 其延迟时间比delay先到期, 什么都不用做.
            if(timer_.IsRunning() && timer_.GetCurrentDelay()<=delay)
            {
                return;
            }

            // 时钟没有运行或者延迟到期很晚, 重新启动.
            timer_.Stop();
            timer_.Start(delay, this, &DelayTimer<Receiver>::Check);
        }

        void Check()
        {
            if(trigger_time_.is_null())
            {
                return;
            }

            // 如果等待的时间不够, 再等一会.
            const Time now = Time::Now();
            if(now < trigger_time_)
            {
                DelayFor(trigger_time_-now);
                return;
            }

            (receiver_->*method_)();
        }

        const Receiver* receiver_;
        const ReceiverMethod method_;
        const TimeDelta delay_;

        OneShotTimer<DelayTimer<Receiver> > timer_;
        Time trigger_time_;
    };

} //namespace base

#endif //__message_timer_h__