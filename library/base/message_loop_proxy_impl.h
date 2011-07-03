
#ifndef __base_message_loop_proxy_impl_h__
#define __base_message_loop_proxy_impl_h__

#pragma once

#include "message_loop.h"
#include "message_loop_proxy.h"

namespace base
{

    // MessageLoopProxy的一个固有实现, 接受MessageLoop, 并通过MessageLoop的
    // DestructionObserver接口跟踪它的生命周期. 目前只能在当前线程创建一个
    // MessageLoopProxyImpl对象.
    class MessageLoopProxyImpl : public MessageLoopProxy,
        public MessageLoop::DestructionObserver
    {
    public:
        virtual ~MessageLoopProxyImpl();

        virtual bool PostTask(Task* task);
        virtual bool PostDelayedTask(Task* task, int64 delay_ms);
        virtual bool PostNonNestableTask(Task* task);
        virtual bool PostNonNestableDelayedTask(Task* task, int64 delay_ms);
        virtual bool BelongsToCurrentThread();

        // MessageLoop::DestructionObserver implementation
        void WillDestroyCurrentMessageLoop();

    protected:
        // 重载OnDestruct, 这样如果目标消息循环还存在, 我们可以删除其中的对象.
        virtual void OnDestruct();

    private:
        MessageLoopProxyImpl();
        bool PostTaskHelper(Task* task, int64 delay_ms, bool nestable);

        friend class MessageLoopProxy;

        // target_message_loop_访问保护锁.
        mutable Lock message_loop_lock_;
        MessageLoop* target_message_loop_;

        DISALLOW_COPY_AND_ASSIGN(MessageLoopProxyImpl);
    };

} //namespace base

#endif //__base_message_loop_proxy_impl_h__