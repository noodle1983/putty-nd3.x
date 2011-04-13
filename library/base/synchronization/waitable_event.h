
#ifndef __base_synchronization_waitable_event_h__
#define __base_synchronization_waitable_event_h__

#pragma once

#include <windows.h>

#include "../basic_types.h"

namespace base
{

    // 替换Win32下的INFINITE.
    static const int kNoTimeout = -1;

    class TimeDelta;

    // WaitableEvent作为线程同步工具, 用于一个线程等待另一个线程完成一件事情.
    // 对于非Windows系统, 只能在同一地址空间中使用.
    //
    // 使用WaitableEvent保护一个boolean值, 否则就要使用Lock+ConditionVariable.
    // 但是当你发现使用WaitableEvent+Lock等待一个更复杂的状态变化时(比如等待一
    // 个元素添加到队列中), 这时可能需要使用ConditionVariable而不是WaitableEvent.
    //
    // 注意: 在Windows上, 类特意提供了一些event对象的功能. 如果编写Windows平台
    // 代码时需要event的另外一些特性, 最好直接使用Windows的event对象.
    class WaitableEvent
    {
    public:
        // 如果manual_reset为true, 设置事件到非信号状态, 用户必须手动调用Reset.
        // 如果参数为false, 在等待线程释放后, 系统会自动重置事件到非信号状态.
        WaitableEvent(bool manual_reset, bool initially_signaled);

        // 通过已经创建的Event句柄创建WaitableEvent. 对象接管句柄的所有权, 析
        // 构时会关闭句柄.
        explicit WaitableEvent(HANDLE event_handle);

        // 从对象中释放句柄的所有权.
        HANDLE Release();

        ~WaitableEvent();

        // 设置Event到非信号状态.
        void Reset();

        // 设置Event到信号状态. 唤醒所有等待线程.
        void Signal();

        // 如果事件处于信号状态返回true, 否则返回false. 如果不是手动重置事件,
        // 函数调用会导致事件重置.
        bool IsSignaled();

        // 无限期地等待直到事件处于信号状态. 如果事件处于信号状态, 返回true,
        // 否则返回false表示等待失败.
        bool Wait();

        // 等待直到事件处于信号状态, max_time为超时时间. 如果事件处于信号状态,
        // 返回true. 函数返回false, 并不一定意味着超时, 有可能等待失败.
        bool TimedWait(const TimeDelta& max_time);

        HANDLE handle() const { return handle_; }

        // 同步等待多个事件.
        //   waitables: 一组WaitableEvent指针
        //   count: @waitables的元素个数
        //
        // 返回值: 处于信号状态的WaitableEvent索引.
        //
        // 在等待的时候不能删除任何WaitableEvent对象.
        static size_t WaitMany(WaitableEvent** waitables, size_t count);

        // 异步等待, 参见WaitableEventWatcher.

        // This is a private helper class. It's here because it's used by friends of
        // this class (such as WaitableEventWatcher) to be able to enqueue elements
        // of the wait-list
        class Waiter
        {
        public:
            // Signal the waiter to wake up.
            //
            // Consider the case of a Waiter which is in multiple WaitableEvent's
            // wait-lists. Each WaitableEvent is automatic-reset and two of them are
            // signaled at the same time. Now, each will wake only the first waiter in
            // the wake-list before resetting. However, if those two waiters happen to
            // be the same object (as can happen if another thread didn't have a chance
            // to dequeue the waiter from the other wait-list in time), two auto-resets
            // will have happened, but only one waiter has been signaled!
            //
            // Because of this, a Waiter may "reject" a wake by returning false. In
            // this case, the auto-reset WaitableEvent shouldn't act as if anything has
            // been notified.
            virtual bool Fire(WaitableEvent* signaling_event) = 0;

            // Waiters may implement this in order to provide an extra condition for
            // two Waiters to be considered equal. In WaitableEvent::Dequeue, if the
            // pointers match then this function is called as a final check. See the
            // comments in ~Handle for why.
            virtual bool Compare(void* tag) = 0;

        protected:
            virtual ~Waiter() {}
        };

    private:
        friend class WaitableEventWatcher;

        HANDLE handle_;

        DISALLOW_COPY_AND_ASSIGN(WaitableEvent);
    };

} //namespace base

#endif //__base_synchronization_waitable_event_h__