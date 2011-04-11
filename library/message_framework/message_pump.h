
#ifndef __message_framework_message_pump_h__
#define __message_framework_message_pump_h__

#pragma once

#include "base/ref_counted.h"

namespace base
{

    class TimeTicks;

    class MessagePump : public RefCountedThreadSafe<MessagePump>
    {
    public:
        // 请参见Run方法的注释, 以了解Delegate的类方法是如何被使用的.
        class Delegate
        {
        public:
            virtual ~Delegate() {}

            // 在Run方法中调用, 负责响应ScheduleWork调度, 如果没有可执行的任务会接着
            // 调用DoDelayedWork. 返回true表示执行任务成功.
            virtual bool DoWork() = 0;

            // 在Run方法中调用, 负责响应ScheduleDelayedWork的调度, 如果没有延迟任务,
            // 消息泵会休眠直到有新的事件需要处理. 返回true表示执行延迟任务成功, 此时
            // 不会调用DoIdleWork(). 根据|next_delayed_work_time|返回值可以确定下一次
            // 调用DoDelayedWork的时间. 如果|next_delayed_work_time|为空(Time::is_null),
            // 表示延迟任务队列为空, 无须再安排该方法的调用.
            virtual bool DoDelayedWork(TimeTicks* next_delayed_work_time) = 0;

            // 在Run方法的消息泵休眠之前调用. 返回true表明空闲任务完成.
            virtual bool DoIdleWork() = 0;
        };

        MessagePump();
        virtual ~MessagePump();

        // 调用Run方法进入消息泵的执行循环.
        //
        // 在Run方法中, 消息泵周期性的处理本地消息并调用代理方法. 消息泵会小心的
        // 处理代理回调和本地消息, 保证两种类型的事件都能够正确的执行.
        //
        // 剖析典型的执行循环:
        //
        //     for(;;) {
        //       bool did_work = DoInternalWork();
        //       if(should_quit_)
        //         break;
        //
        //       did_work |= delegate_->DoWork();
        //       if(should_quit_)
        //         break;
        //
        //       did_work |= delegate_->DoDelayedWork();
        //       if(should_quit_)
        //         break;
        //
        //       if(did_work)
        //         continue;
        //
        //       did_work = delegate_->DoIdleWork();
        //       if(should_quit_)
        //         break;
        //
        //       if(did_work)
        //         continue;
        //
        //       WaitForWork();
        //     }
        //
        // 其中, DoInternalWork是消息泵的私有方法, 负责派发UI消息或者通知IO完成.
        // WaitForWork私有方法堵塞线程, 直到有新的事件需要处理.
        //
        // DoInternalWork、DoWork和DoDelayedWork函数在循环中不断地执行, 确保两种
        // 类型的任务队列都有机会被执行. 举例来说, 这对驱动动画的消息泵非常重要.
        //
        // 值得注意的是每次调用外部代码, 循环都会检查是否需要退出. Quit方法负责设
        // 置标志位. 一旦设置了退出标志位, 不再处理任何事情.
        //
        // 注意: 外部调用流程中的代码再次调用Run方法必须非常小心. 本地消息泵也可能
        // 需要处理在别处运行的本地消息泵(比如 Windows上的MessageBox API会发送UI消
        // 息!). 明确地讲,嵌套的子循环表面上看来不受消息泵控制, 但仍须提供外部调用
        // 流程(DoWork和DoDelayedWork). DoWork必须获取时间片, 除非返回false(没有事
        // 情可做了).
        virtual void Run(Delegate* delegate) = 0;

        // 尽快终止最内层的执行循环. 只能在调用Run方法的线程中调用.
        virtual void Quit() = 0;

        // 调度尽快的执行一次DoWork回调, 如果已调度过则什么都不做. 方法可以在任意
        // 线程调用. 一旦调用ScheduleWork函数, DoWork在返回false前必定会执行.
        virtual void ScheduleWork() = 0;

        // 调度在某一时间执行一次DoDelayedWork回调, 取消所有等待的DoDelayedWork回调.
        // 只能在调用Run方法的线程中使用.
        virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time) = 0;
    };

} //namespace base

#endif //__message_framework_message_pump_h__