
#include "message_pump_win.h"

#include <math.h>

#include "base/win/wrapped_window_proc.h"

namespace base
{

    static const wchar_t kWndClass[] = L"Wan_MessagePumpWindow";

    // 发送kMsgHaveWork消息, 从消息泵获取时间片, 处理下一个任务(接连不断的发送形成
    // 连续的任务泵).
    static const int kMsgHaveWork = WM_USER + 1;

    void MessagePumpWin::AddObserver(Observer* observer)
    {
        observers_.AddObserver(observer);
    }

    void MessagePumpWin::RemoveObserver(Observer* observer)
    {
        observers_.RemoveObserver(observer);
    }

    void MessagePumpWin::WillProcessMessage(const MSG& msg)
    {
        FOR_EACH_OBSERVER(Observer, observers_, WillProcessMessage(msg));
    }

    void MessagePumpWin::DidProcessMessage(const MSG& msg)
    {
        FOR_EACH_OBSERVER(Observer, observers_, DidProcessMessage(msg));
    }

    void MessagePumpWin::RunWithDispatcher(Delegate* delegate,
        Dispatcher* dispatcher)
    {
        RunState s;
        s.delegate = delegate;
        s.dispatcher = dispatcher;
        s.should_quit = false;
        s.run_depth = state_ ? state_->run_depth+1 : 1;

        RunState* previous_state = state_;
        state_ = &s;

        DoRunLoop();

        state_ = previous_state;
    }

    void MessagePumpWin::Quit()
    {
        DCHECK(state_);
        state_->should_quit = true;
    }

    int MessagePumpWin::GetCurrentDelay() const
    {
        if(delayed_work_time_.is_null())
        {
            return -1;
        }

        // 这里需要小心. TimeDelta精度是微秒, 但这里只需要毫秒值. 如果剩余5.5ms, 应该
        // 延迟5ms还是6ms呢? 为避免过早的执行延迟任务, 应该是6ms.
        double timeout = ceil((delayed_work_time_ - TimeTicks::Now()).InMillisecondsF());

        // 如果是负数, 应该立即执行延迟任务.
        int delay = static_cast<int>(timeout);
        if(delay < 0)
        {
            delay = 0;
        }

        return delay;
    }

    MessagePumpForUI::MessagePumpForUI()
    {
        InitMessageWnd();
    }

    MessagePumpForUI::~MessagePumpForUI()
    {
        DestroyWindow(message_hwnd_);
        UnregisterClass(kWndClass, GetModuleHandle(NULL));
    }

    void MessagePumpForUI::ScheduleWork()
    {
        if(InterlockedExchange(&have_work_, 1))
        {
            return; // 别处的消息泵在运行.
        }

        // 确保MessagePump会执行任务.
        PostMessage(message_hwnd_, kMsgHaveWork,
            reinterpret_cast<WPARAM>(this), 0);
    }

    void MessagePumpForUI::ScheduleDelayedWork(const TimeTicks& delayed_work_time)
    {
        // 可以提供高精度的时钟. Windows的时钟使用SetTimer(), 精度为10ms. 我们必须
        // 使用WM_TIMER作为唤醒机制, 因为程序可能会进入模态循环而不执行MessageLoop;
        // 触发时钟的唯一方法就是通过消息投递.
        //
        // 为了提供精度高于10ms的时钟, 需要在自己的执行循环中进行时钟处理. 一般来讲,
        // 执行循环的工作流程中会处理时钟. 但仍需要设置了一个系统时钟, 弥补模态消息循
        // 环中无法执行本循环的情况. 有可能触发SetTimer时没有等待时钟任务, 因为执行
        // 循环可能已经处理完了.
        //
        // 使用SetTimer设置一个尽快触发的时钟. 随着新的时钟创建销毁, 会更新SetTimer.
        // 触发一个假的SetTimer事件是无害的, 因为可能只是处理一个空的时钟队列.
        delayed_work_time_ = delayed_work_time;

        int delay_msec = GetCurrentDelay();
        DCHECK(delay_msec >= 0);
        if(delay_msec < USER_TIMER_MINIMUM)
        {
            delay_msec = USER_TIMER_MINIMUM;
        }

        // 创建WM_TIMER事件会唤醒消息泵检查等待的时钟任务(在嵌套执行的时候通过子循环).
        SetTimer(message_hwnd_, reinterpret_cast<UINT_PTR>(this), delay_msec, NULL);
    }

    void MessagePumpForUI::PumpOutPendingPaintMessages()
    {
        // 在Run函数之外调用, 不做任何处理.
        if(!state_)
        {
            return;
        }

        // 创建一个小的消息泵强制尽快处理WM_PAINT消息. 虽然不提供无限循环, 但是足够
        // 处理所有事情. 一般最多4次就可以, 这里用20次更安全一些.
        const int kMaxPeekCount = 20;
        int peek_count;
        for(peek_count=0; peek_count<kMaxPeekCount; ++peek_count)
        {
            MSG msg;
            if(!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE|PM_QS_PAINT))
            {
                break;
            }
            ProcessMessageHelper(msg);
            if(state_->should_quit) // 处理WM_QUIT.
            {
                break;
            }
        }
    }

    // static
    LRESULT CALLBACK MessagePumpForUI::WndProcThunk(
        HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
    {
        switch(message)
        {
        case kMsgHaveWork:
            reinterpret_cast<MessagePumpForUI*>(wparam)->HandleWorkMessage();
            break;
        case WM_TIMER:
            reinterpret_cast<MessagePumpForUI*>(wparam)->HandleTimerMessage();
            break;
        }
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    void MessagePumpForUI::DoRunLoop()
    {
        // 如果只是简单的PeekMessage()循环, 依据MSDN关于PeekMessage的文档, Windows
        // 将会按照下面的顺序处理消息队列中所有的任务, 不进行任何过滤:
        //     * Sent messages
        //     * Posted messages
        //     * Sent messages (again)
        //     * WM_PAINT messages
        //     * WM_TIMER messages
        //
        // 总结: 上面所有的消息都会得到执行, Sent的消息有2次处理机会(可减少服务时间).

        for(;;)
        {
            // 在处理任务的时候, 可能会创建一些消息, 这样等待队列中就会有更多待处理的
            // 任务. 比如ProcessNextWindowsMessage()时, 可能还有更多的消息在等待. 换句
            // 话说, 某一次没有处理任务, 再次调用可能也不会有任务在等待. 最后, 如果没有
            // 任何可处理的任务, 此时可以考虑休眠等待任务.
            bool more_work_is_plausible = ProcessNextWindowsMessage();
            if(state_->should_quit)
            {
                break;
            }

            more_work_is_plausible |= state_->delegate->DoWork();
            if(state_->should_quit)
            {
                break;
            }

            more_work_is_plausible |= state_->delegate->DoDelayedWork(
                &delayed_work_time_);
            // 如果有没被处理的延迟任务, 那么我们需要WM_TIMER的存在触发执行延迟任务.
            // 如果时钟正在运行, 我们不要去停止它. 但是如果所有延迟任务都被处理了,
            // 需要停止WM_TIMER.
            if(more_work_is_plausible && delayed_work_time_.is_null())
            {
                KillTimer(message_hwnd_, reinterpret_cast<UINT_PTR>(this));
            }
            if(state_->should_quit)
            {
                break;
            }

            if(more_work_is_plausible)
            {
                continue;
            }

            more_work_is_plausible = state_->delegate->DoIdleWork();
            if(state_->should_quit)
            {
                break;
            }

            if(more_work_is_plausible)
            {
                continue;
            }

            WaitForWork(); // 等待(休眠)直到有任务到来.
        }
    }

    void MessagePumpForUI::InitMessageWnd()
    {
        HINSTANCE hinst = GetModuleHandle(NULL);

        WNDCLASSEX wc = { 0 };
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = base::WrappedWindowProc<WndProcThunk>;
        wc.hInstance = hinst;
        wc.lpszClassName = kWndClass;
        RegisterClassEx(&wc);

        message_hwnd_ = CreateWindow(kWndClass, 0, 0, 0, 0, 0, 0,
            HWND_MESSAGE, 0, hinst, 0);
        DCHECK(message_hwnd_);
    }

    void MessagePumpForUI::WaitForWork()
    {
        // 等待直到有消息到来, 触发下一组时钟的时间取决于时钟管理.
        int delay = GetCurrentDelay();
        if(delay < 0) // 负数表示没有等待时钟.
        {
            delay = INFINITE;
        }

        DWORD result;
        result = MsgWaitForMultipleObjectsEx(0, NULL, delay,
            QS_ALLINPUT, MWMO_INPUTAVAILABLE);

        if(WAIT_OBJECT_0 == result)
        {
            // WM_*消息到达.
            // 如果父子窗口分别在两个线程, 他们的输入都会经过消息队列.
            // 导致MsgWaitForMultipleObjectsEx返回的原因是消息已经准备好, 等待处理
            // (特指子窗口的鼠标消息, 子窗口当前被捕获).
            // 连续调用PeekMessages返回消息失败会导致死循环.
            // 调用WaitMessage给子窗口一些时间处理输入消息.
#ifndef WM_NCMOUSEFIRST
#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#endif
#ifndef WM_NCMOUSELAST
#define WM_NCMOUSELAST  WM_NCXBUTTONDBLCLK
#endif
            MSG msg = { 0 };
            DWORD queue_status = GetQueueStatus(QS_MOUSE);
            if(HIWORD(queue_status)&QS_MOUSE && !PeekMessage(&msg, NULL,
                WM_MOUSEFIRST, WM_MOUSELAST, PM_NOREMOVE) && !PeekMessage(&msg,
                NULL, WM_NCMOUSEFIRST, WM_NCMOUSELAST, PM_NOREMOVE))
            {
                WaitMessage();
            }
            return;
        }

        DCHECK_NE(WAIT_FAILED, result) << GetLastError();
    }

    void MessagePumpForUI::HandleWorkMessage()
    {
        // 如果在Run函数之外调用, 不做任何处理. 对应于MessageBox调用或者其它类似情况.
        if(!state_)
        {
            // 因为处理了kMsgHaveWork消息, 所以需要更新标志位.
            InterlockedExchange(&have_work_, 0);
            return;
        }

        // 防止无用的消息影响Windows消息队列中正常的消息的处理.
        ProcessPumpReplacementMessage();

        // 让代理得到执行机会, 通过返回值可以知道是否需要处理更多的任务.
        if(state_->delegate->DoWork())
        {
            ScheduleWork();
        }
    }

    void MessagePumpForUI::HandleTimerMessage()
    {
        KillTimer(message_hwnd_, reinterpret_cast<UINT_PTR>(this));

        // 如果在Run函数之外调用, 不做任何处理. 对应于MessageBox调用或者其它类似情况.
        if(!state_)
        {
            return;
        }

        state_->delegate->DoDelayedWork(&delayed_work_time_);
        if(!delayed_work_time_.is_null())
        {
            // 再次设置delayed_work_time_.
            ScheduleDelayedWork(delayed_work_time_);
        }
    }

    bool MessagePumpForUI::ProcessNextWindowsMessage()
    {
        // 如果消息队列中有Sent消息, PeekMessage内部会分发消息并返回false.
        // 为确保再次察看消息循环而不是调用MsgWaitForMultipleObjectsEx, 函数返
        // 回true.
        bool sent_messages_in_queue = false;
        DWORD queue_status = GetQueueStatus(QS_SENDMESSAGE);
        if(HIWORD(queue_status) & QS_SENDMESSAGE)
        {
            sent_messages_in_queue = true;
        }

        MSG msg;
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            return ProcessMessageHelper(msg);
        }

        return sent_messages_in_queue;
    }

    bool MessagePumpForUI::ProcessMessageHelper(const MSG& msg)
    {
        if(WM_QUIT == msg.message)
        {
            // 再次发送QUIT消息, 以便主循环GetMessage()能够接收到.
            state_->should_quit = true;
            PostQuitMessage(static_cast<int>(msg.wParam));
            return false;
        }

        // 在主消息泵中执行时, 丢弃kMsgHaveWork消息.
        if(msg.message==kMsgHaveWork && msg.hwnd==message_hwnd_)
        {
            return ProcessPumpReplacementMessage();
        }

        if(CallMsgFilter(const_cast<MSG*>(&msg), kMessageFilterCode))
        {
            return true;
        }

        WillProcessMessage(msg);

        if(state_->dispatcher)
        {
            if(!state_->dispatcher->Dispatch(msg))
            {
                state_->should_quit = true;
            }
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        DidProcessMessage(msg);
        return true;
    }

    bool MessagePumpForUI::ProcessPumpReplacementMessage()
    {
        // 处理kMsgHaveWork消息的时候调用本方法. 察看并处理可替换的消息, 比如WM_PAINT
        // 或WM_TIMER. 目的是为了使kMsgHaveWork尽量不干扰Windows自身的消息机制, 即使
        // 这种消息被不断的发送. 方法小心地获取一个消息, 该消息不可能是kMsgHaveWork,
        // 重置have_work_标志位(允许再次发送kMsgHaveWork消息), 最后分发取到的消息.
        // 注意对这个线程再次发送kMsgHaveWork是异步的!
        MSG msg;
        bool have_message = (0 != PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));
        DCHECK(!have_message || kMsgHaveWork!=msg.message ||
            msg.hwnd!=message_hwnd_);

        // 因为丢弃了kMsgHaveWork消息, 必须更新标志位.
        int old_have_work = InterlockedExchange(&have_work_, 0);
        DCHECK(old_have_work);

        // 如果没有消息需要处理, 不再需要执行时间片.
        if(!have_message)
        {
            return false;
        }

        // 保证进入本地消息代码后能得到时间片. 任务非常少的时候, ScheduleWork()有一点
        // 影响性能, 但是当事件队列繁忙的时候, kMsgHaveWork会变得越来越少(相对).
        ScheduleWork();
        return ProcessMessageHelper(msg);
    }

    MessagePumpForIO::MessagePumpForIO()
    {
        port_.Set(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1));
        DCHECK(port_.IsValid());
    }

    void MessagePumpForIO::ScheduleWork()
    {
        if(InterlockedExchange(&have_work_, 1))
        {
            return; // 别处的消息泵在运行.
        }

        // 确保MessagePump会执行任务.
        BOOL ret = PostQueuedCompletionStatus(port_, 0,
            reinterpret_cast<ULONG_PTR>(this),
            reinterpret_cast<OVERLAPPED*>(this));
        DCHECK(ret);
    }

    void MessagePumpForIO::ScheduleDelayedWork(const TimeTicks& delayed_work_time)
    {
        // 由于方法只能在Run的同一线程中被调用, 函数不会被堵塞. 这里只需要更新休眠
        // 的时间.
        delayed_work_time_ = delayed_work_time;
    }

    void MessagePumpForIO::RegisterIOHandler(HANDLE file_handle,
        IOHandler* handler)
    {
        ULONG_PTR key = reinterpret_cast<ULONG_PTR>(handler);
        HANDLE port = CreateIoCompletionPort(file_handle, port_, key, 1);
        DCHECK(port == port_.Get());
    }

    void MessagePumpForIO::DoRunLoop()
    {
        for(;;)
        {
            // 在处理任务的时候, 可能会创建一些消息, 这样等待队列中就会有更多待处理的
            // 任务. 比如当WaitForIOCompletion()时, 可能还有更多的消息在等待. 换句话
            // 说, 某一次没有处理任务, 再次调用可能也不会有任务在等待. 最后, 如果没有
            // 任何可处理的任务, 此时可以考虑休眠等待任务.
            bool more_work_is_plausible = state_->delegate->DoWork();
            if(state_->should_quit)
            {
                break;
            }

            more_work_is_plausible |= WaitForIOCompletion(0, NULL);
            if(state_->should_quit)
            {
                break;
            }

            more_work_is_plausible |=
                state_->delegate->DoDelayedWork(&delayed_work_time_);
            if(state_->should_quit)
            {
                break;
            }

            if(more_work_is_plausible)
            {
                continue;
            }

            more_work_is_plausible = state_->delegate->DoIdleWork();
            if(state_->should_quit)
            {
                break;
            }

            if(more_work_is_plausible)
            {
                continue;
            }

            WaitForWork(); // 等待(休眠)直到有任务到来.
        }
    }

    // 等待直到有消息到来, 触发下一组时钟的时间取决于时钟管理.
    void MessagePumpForIO::WaitForWork()
    {
        // IO消息循环不支持嵌套. 避免递归带来的麻烦.
        DCHECK(state_->run_depth == 1) << "Cannot nest an IO message loop!";

        int timeout = GetCurrentDelay();
        if(timeout < 0) // 负数表示没有等待时钟.
        {
            timeout = INFINITE;
        }

        WaitForIOCompletion(timeout, NULL);
    }

    bool MessagePumpForIO::WaitForIOCompletion(DWORD timeout, IOHandler* filter)
    {
        IOItem item;
        if(completed_io_.empty() || !MatchCompletedIOItem(filter, &item))
        {
            // 向系统请求下一个IO完成.
            if(!GetIOItem(timeout, &item))
            {
                return false;
            }

            if(ProcessInternalIOItem(item))
            {
                return true;
            }
        }

        if(item.context->handler)
        {
            if(filter && item.handler!=filter)
            {
                // 保存到后面.
                completed_io_.push_back(item);
            }
            else
            {
                DCHECK_EQ(item.context->handler, item.handler);
                WillProcessIOEvent();
                item.handler->OnIOCompleted(item.context,
                    item.bytes_transfered, item.error);
                DidProcessIOEvent();
            }
        }
        else
        {
            // 此时需要清除handler.
            delete item.context;
        }
        return true;
    }

    // 向OS再次请求一个IO完成结果.
    bool MessagePumpForIO::GetIOItem(DWORD timeout, IOItem* item)
    {
        memset(item, 0, sizeof(*item));
        ULONG_PTR key = NULL;
        OVERLAPPED* overlapped = NULL;
        if(!GetQueuedCompletionStatus(port_.Get(), &item->bytes_transfered,
            &key, &overlapped, timeout))
        {
            if(!overlapped)
            {
                return false; // 队列中什么都没有.
            }
            item->error = GetLastError();
            item->bytes_transfered = 0;
        }

        item->handler = reinterpret_cast<IOHandler*>(key);
        item->context = reinterpret_cast<IOContext*>(overlapped);
        return true;
    }

    bool MessagePumpForIO::ProcessInternalIOItem(const IOItem& item)
    {
        if(this==reinterpret_cast<MessagePumpForIO*>(item.context) &&
            this==reinterpret_cast<MessagePumpForIO*>(item.handler))
        {
            // 内部的完成事件.
            DCHECK(!item.bytes_transfered);
            InterlockedExchange(&have_work_, 0);
            return true;
        }
        return false;
    }

    // 返回一个接收到的完成事件.
    bool MessagePumpForIO::MatchCompletedIOItem(IOHandler* filter, IOItem* item)
    {
        DCHECK(!completed_io_.empty());
        for(std::list<IOItem>::iterator it=completed_io_.begin();
            it!=completed_io_.end(); ++it)
        {
            if(!filter || it->handler==filter)
            {
                *item = *it;
                completed_io_.erase(it);
                return true;
            }
        }
        return false;
    }

    void MessagePumpForIO::AddIOObserver(IOObserver* obs)
    {
        io_observers_.AddObserver(obs);
    }

    void MessagePumpForIO::RemoveIOObserver(IOObserver* obs)
    {
        io_observers_.RemoveObserver(obs);
    }

    void MessagePumpForIO::WillProcessIOEvent()
    {
        FOR_EACH_OBSERVER(IOObserver, io_observers_, WillProcessIOEvent());
    }

    void MessagePumpForIO::DidProcessIOEvent()
    {
        FOR_EACH_OBSERVER(IOObserver, io_observers_, DidProcessIOEvent());
    }

} //namespace base