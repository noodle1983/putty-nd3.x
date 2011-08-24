
#ifndef __base_message_pump_default_h__
#define __base_message_pump_default_h__

#pragma once

#include "message_pump.h"
#include "synchronization/waitable_event.h"
#include "time.h"

namespace base
{

    class MessagePumpDefault : public MessagePump
    {
    public:
        MessagePumpDefault();
        ~MessagePumpDefault() {}

        virtual void Run(Delegate* delegate);
        virtual void Quit();
        virtual void ScheduleWork();
        virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time);

    private:
        // 标志位设置为false表示Run循环需要返回.
        bool keep_running_;

        // 休眠直到有事情发生.
        WaitableEvent event_;

        // 需要调用DoDelayedWork的时间点.
        TimeTicks delayed_work_time_;

        DISALLOW_COPY_AND_ASSIGN(MessagePumpDefault);
    };

} //namespace base

#endif //__base_message_pump_default_h__