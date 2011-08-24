
#include "timer.h"

#include "message_loop.h"

namespace base
{

    void BaseTimer_Helper::OrphanDelayedTask()
    {
        if(delayed_task_)
        {
            delayed_task_->timer_ = NULL;
            delayed_task_ = NULL;
        }
    }

    void BaseTimer_Helper::InitiateDelayedTask(TimerTask* timer_task)
    {
        OrphanDelayedTask();

        delayed_task_ = timer_task;
        delayed_task_->timer_ = this;
        MessageLoop::current()->PostDelayedTask(timer_task,
            static_cast<int>(timer_task->delay_.InMillisecondsRoundedUp()));
    }

} //namespace base