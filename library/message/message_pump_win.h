
#ifndef __message_message_pump_win_h__
#define __message_message_pump_win_h__

#pragma once

#include <windows.h>

#include <list>

#include "base/time.h"
#include "base/win/scoped_handle.h"

#include "message_pump.h"
#include "observer_list.h"

namespace base
{

    // MessagePumpWin是MessagePump在Windows下实现的基类. 提供了一些基本功能, 像观察者
    // 的处理以及消息泵生命周期的控制.
    class MessagePumpWin : public MessagePump
    {
    public:
        // Observer对象接收来自UI消息循环的全局通知消息.
        //
        // 注意: 观察者的实现应该是非常快的!
        class Observer
        {
        public:
            virtual ~Observer() {}

            // 处理UI消息前调用, 可能是未定义消息(msg.message为0).
            virtual void WillProcessMessage(const MSG& msg) = 0;

            // 处理UI消息后调用, 可能是未定义消息(msg.message为0).
            virtual void DidProcessMessage(const MSG& msg) = 0;
        };

        // 嵌套调用的Run使用Dispatcher来派发事件. 如果调用Run时传递的Dispatcher不
        // 为空, MessageLoop不会派发事件(调用TranslateMessage), 所有的消息传递给
        // Dispatcher的Dispatch方法. 事件是否派发取决于Dispatcher.
        //
        // 嵌套的循环通过post一个quit消息或者Dispatch返回false退出.
        class Dispatcher
        {
        public:
            virtual ~Dispatcher() {}
            // 派发事件. 返回true表示继续, 返回false时嵌套的循环立即退出.
            virtual bool Dispatch(const MSG& msg) = 0;
        };

        MessagePumpWin() : have_work_(0), state_(NULL) {}
        virtual ~MessagePumpWin() {}

        // 添加一个Observer, 将立即接收通知消息.
        void AddObserver(Observer* observer);

        // 移除一个Observer, 该方法在当前Observer接到通知回调的时候调用也是安全的.
        void RemoveObserver(Observer* observer);

        // 通知消息循环的观察者将要处理一个消息.
        void WillProcessMessage(const MSG& msg);
        void DidProcessMessage(const MSG& msg);

        // 和MessagePump::Run类似, 如果dispatcher不为空则MSG对象通过它路由.
        void RunWithDispatcher(Delegate* delegate, Dispatcher* dispatcher);

        virtual void Run(Delegate* delegate) { RunWithDispatcher(delegate, NULL); }
        virtual void Quit();

    protected:
        struct RunState
        {
            Delegate* delegate;
            Dispatcher* dispatcher;

            // Run()调用尽快返回的标志位.
            bool should_quit;

            // Run()嵌套调用堆栈的深度.
            int run_depth;
        };

        virtual void DoRunLoop() = 0;
        int GetCurrentDelay() const;

        ObserverList<Observer> observers_;

        // 延迟任务执行的时间点.
        TimeTicks delayed_work_time_;

        // 标记kMsgDoWork是否已经在Windows消息队列中. 这个消息最多只有一个, 在本地
        // 消息泵运行的同时驱动任务的执行.
        LONG have_work_;

        // 当前Run调用的状态.
        RunState* state_;
    };

    //-----------------------------------------------------------------------------
    // MessagePumpForUI派生自MessagePumpWin, 配合用TYPE_UI初始化的MessageLoop工作.
    //
    // MessagePumpForUI实现传统的Windows消息泵. 通过一个无限循环, 不断地取消息并派
    // 发. 取消息的同时调用DoWork处理等待的任务, 调用DoDelayedWork处理等待的时钟.
    // 没有事件处理的时候, 消息泵进入等待状态. 大部分情况下, 所有的事件和任务都由
    // MessagePumpForUI处理.
    //
    // 但是, 当一个任务或者事件在栈上创建了一个本地的对话框, 这个窗口只会提供一个
    // 本地的消息泵. 这个消息泵仅仅是从Windows消息队列取消息, 随后进行消息的派发.
    // MessageLoop对本地的消息泵进行了扩展以支持任务服务, 只是有一些复杂.
    //
    // 子消息泵的扩展实现的基本结构是一个特殊的消息kMsgHaveWork, 它被不断地注入到
    // Windows消息队列. 每次见到kMsgHaveWork消息, 会进行一系列事件的检查, 包括是否
    // 有可运行的任务.
    //
    // 一个任务执行完成后, kMsgHaveWork消息再次被投递到Windows消息队列, 确保后面的
    // 事件还能获得时间片. 为防止kMsgHaveWork太多导致Windows消息队列溢出, 需要保证
    // Window的消息队列最多只能有一个kMsgHaveWork消息.
    //
    // 当没有任务运行的时候, 系统的复杂性在于如何使得子消息泵的驱动消息不停止. 消息
    // 泵在队列有任务的时候会自动起动.
    //
    // 第二个复杂性在于连续不断地投递任务会阻止Windows消息泵处理WM_PAINT或者
    // WM_TIMER. 这种绘图和时钟消息往往比kMsgHaveWork消息优先级高. 所以, 在每次投递
    // kMsgHaveWork消息之间(比如在获取kMsgHaveWork消息之后和再次投递之前)处理一些其
    // 它消息.
    //
    // 注意: 尽管使用消息来启动或者停止流程(与信号对象相反)看起来很奇怪, 但是本地
    // 消息泵只能响应消息, 这样就好理解了. 所以, 这是一个还不错的选择. 这也有助于
    // 新任务在队列中投递的启动消息唤醒DoRunLoop.
    class MessagePumpForUI : public MessagePumpWin
    {
    public:
        // 应用程序定义的传递给钩子过程的代码.
        static const int kMessageFilterCode = 0x5001;

        MessagePumpForUI();
        virtual ~MessagePumpForUI();

        virtual void ScheduleWork();
        virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time);

        // 程序调用处理所有等待的WM_PAINT消息. 本方法将会处理Windows消息队列中所有的
        // 绘图消息, 具体取决于一个固定的数目(避免无限循环).
        void PumpOutPendingPaintMessages();

    private:
        static LRESULT CALLBACK WndProcThunk(
            HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
        virtual void DoRunLoop();
        void InitMessageWnd();
        void WaitForWork();
        void HandleWorkMessage();
        void HandleTimerMessage();
        bool ProcessNextWindowsMessage();
        bool ProcessMessageHelper(const MSG& msg);
        bool ProcessPumpReplacementMessage();

        // 用于消息的隐藏窗口.
        HWND message_hwnd_;
    };

    //-----------------------------------------------------------------------------
    // MessagePumpForIO派生自MessagePumpWin, 配合用TYPE_IO初始化的MessageLoop工作.
    // 这个版本的MessagePump不处理Windows消息, 它的执行循环是基于完成端口的, 所以
    // 比较适合IO操作.
    class MessagePumpForIO : public MessagePumpWin
    {
    public:
        struct IOContext;

        // 假如想要接收OS异步IO操作完成通知, 你可以实现IOHandler接口, 并把自己注册到
        // 消息泵.
        //
        // 用法1:
        //     // 实际IO不牵扯到用户缓冲区时使用, 清理工作由消息泵自动完成.
        //     class MyFile : public IOHandler {
        //       MyFile() {
        //         ...
        //         context_ = new IOContext;
        //         context_->handler = this;
        //         message_pump->RegisterIOHandler(file_, this);
        //       }
        //       ~MyFile() {
        //         if(pending_) {
        //           // 设置handler为NULL, 在接到通知时请求删除context, 不再回调.
        //           context_->handler = NULL;
        //         } else {
        //           delete context_;
        //         }
        //       }
        //       virtual void OnIOCompleted(IOContext* context, DWORD bytes_transfered,
        //         DWORD error) {
        //           pending_ = false;
        //       }
        //       void DoSomeIo() {
        //         ...
        //         // 操作需要的唯一缓冲区是overlapped结构体.
        //         ConnectNamedPipe(file_, &context_->overlapped);
        //         pending_ = true;
        //       }
        //       bool pending_;
        //       IOContext* context_;
        //       HANDLE file_;
        //     };
        //
        // 用法2:
        //     class MyFile : public IOHandler {
        //       MyFile() {
        //         ...
        //         message_pump->RegisterIOHandler(file_, this);
        //       }
        //       // 添加一些代码, 确保有等待的IO操作时对象不会被析构.
        //       ~MyFile() {
        //       }
        //       virtual void OnIOCompleted(IOContext* context, DWORD bytes_transfered,
        //         DWORD error) {
        //         ...
        //         delete context;
        //       }
        //       void DoSomeIo() {
        //         ...
        //         IOContext* context = new IOContext;
        //         // 代码什么都不做, 只是防止context被丢弃.
        //         context->handler = this;
        //         ReadFile(file_, buffer, num_bytes, &read, &context->overlapped);
        //       }
        //       HANDLE file_;
        //     };
        //
        // 用法3:
        // 和前面一样, 只是在析构的时候调用WaitForIOCompletion等待所有的IO完成.
        //     ~MyFile() {
        //       while(pending_)
        //         message_pump->WaitForIOCompletion(INFINITE, this);
        //     }
        class IOHandler
        {
        public:
            virtual ~IOHandler() {}
            // 一旦|context|关联的IO操作完成, 函数会被调用. |error|是Win32的IO操作
            // 的错误码(ERROR_SUCCESS表示没错误). 发生错误时|bytes_transfered|为0.
            virtual void OnIOCompleted(IOContext* context, DWORD bytes_transfered,
                DWORD error) = 0;
        };

        // IOObserver对象从MessagePump接收IO通知消息.
        //
        // 注意: IOObserver的实现应该是非常快的!
        class IOObserver
        {
        public:
            IOObserver() {}

            virtual void WillProcessIOEvent() = 0;
            virtual void DidProcessIOEvent() = 0;

        protected:
            virtual ~IOObserver() {}
        };

        // 所有重叠IO操作都需要从这个基类派生. 启动操作时, 需要设置|handler|为文件注册
        // 的IOHandler, 在操作完成前可以设置|handler|为NULL表示不需要再调用处理函数.
        // OS通知操作完成时需要删除IOContext. 由于所有IO操作牵扯到的缓冲区在回调执行
        // 之前一直被占用, 这种技术只能用于不需要额外缓冲区的IO(重叠结构体本身除外).
        struct IOContext
        {
            OVERLAPPED overlapped;
            IOHandler* handler;
        };

        MessagePumpForIO();
        virtual ~MessagePumpForIO() {}

        virtual void ScheduleWork();
        virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time);

        // 注册处理器处理文件的异步IO完成事件. 只要|file_handle|合法, 注册一直有效,
        // 所以只要有等待的文件IO, |handler|就必须合法.
        void RegisterIOHandler(HANDLE file_handle, IOHandler* handler);

        // 等待下一个由|filter|处理的IO完成事件, 等待超时为|timeout|毫秒. 如果IO操作
        // 完成返回true, 超时返回false. 如果完成端口收到消息, 且IO处理器为|filter|,
        // 回调函数在代码返回之前执行; 如果处理器不是|filter|, 回调会延缓执行, 这样就
        // 避免了重入的问题. 只有当调用者允许本线程暂停任务派发这种少见的情况下, 才可
        // 以外面使用本方法.
        bool WaitForIOCompletion(DWORD timeout, IOHandler* filter);

        void AddIOObserver(IOObserver* obs);
        void RemoveIOObserver(IOObserver* obs);

    private:
        struct IOItem
        {
            IOHandler* handler;
            IOContext* context;
            DWORD bytes_transfered;
            DWORD error;
        };

        virtual void DoRunLoop();
        void WaitForWork();
        bool MatchCompletedIOItem(IOHandler* filter, IOItem* item);
        bool GetIOItem(DWORD timeout, IOItem* item);
        bool ProcessInternalIOItem(const IOItem& item);
        void WillProcessIOEvent();
        void DidProcessIOEvent();

        // 线程关联的完成端口.
        ScopedHandle port_;
        // 此列表总是为空. 它在对象清理时保存有还未投递的IO完成事件.
        std::list<IOItem> completed_io_;

        ObserverList<IOObserver> io_observers_;
    };

} //namespace base

#endif //__message_message_pump_win_h__