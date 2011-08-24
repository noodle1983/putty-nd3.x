
#include "message_pump_default.h"

#include "logging.h"

namespace base
{

    MessagePumpDefault::MessagePumpDefault()
        : keep_running_(true), event_(false, false) {}

    void MessagePumpDefault::Run(Delegate* delegate)
    {
        DCHECK(keep_running_) << "Quit must have been called outside of Run!";

        for(;;)
        {
            bool did_work = delegate->DoWork();
            if(!keep_running_)
            {
                break;
            }

            did_work |= delegate->DoDelayedWork(&delayed_work_time_);
            if(!keep_running_)
            {
                break;
            }

            if(did_work)
            {
                continue;
            }

            did_work = delegate->DoIdleWork();
            if(!keep_running_)
            {
                break;
            }

            if(did_work)
            {
                continue;
            }

            if(delayed_work_time_.is_null())
            {
                event_.Wait();
            }
            else
            {
                TimeDelta delay = delayed_work_time_ - TimeTicks::Now();
                if(delay > TimeDelta())
                {
                    event_.TimedWait(delay);
                }
                else
                {
                    // delayed_work_time_表示一个过去的时间, 所以立即调用DoDelayedWork.
                    delayed_work_time_ = TimeTicks();
                }
            }
            // 由于event_自动重置, 这里除了调用代理方法以外不需要做其它事情.
        }

        keep_running_ = true;
    }

    void MessagePumpDefault::Quit()
    {
        keep_running_ = false;
    }

    void MessagePumpDefault::ScheduleWork()
    {
        // 因为会被任意线程调用, 我们需要保证唤醒Run循环.
        event_.Signal();
    }

    void MessagePumpDefault::ScheduleDelayedWork(const TimeTicks& delayed_work_time)
    {
        // 由于方法只能在Run的同一线程中被调用, 函数不会被堵塞. 这里只需要更新休眠
        // 的时间.
        delayed_work_time_ = delayed_work_time;
    }

} //namespace base