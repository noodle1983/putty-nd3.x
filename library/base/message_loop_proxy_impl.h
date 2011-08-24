
#ifndef __base_message_loop_proxy_impl_h__
#define __base_message_loop_proxy_impl_h__

#pragma once

#include "message_loop.h"
#include "message_loop_proxy.h"

namespace base
{

    // A stock implementation of MessageLoopProxy that is created and managed by a
    // MessageLoop. For now a MessageLoopProxyImpl can only be created as part of a
    // MessageLoop.
    class MessageLoopProxyImpl : public MessageLoopProxy
    {
    public:
        virtual ~MessageLoopProxyImpl();

        virtual bool PostTask(Task* task);
        virtual bool PostDelayedTask(Task* task, int64 delay_ms);
        virtual bool PostNonNestableTask(Task* task);
        virtual bool PostNonNestableDelayedTask(Task* task, int64 delay_ms);
        virtual bool PostTask(const base::Closure& task);
        virtual bool PostDelayedTask(const base::Closure& task, int64 delay_ms);
        virtual bool PostNonNestableTask(const base::Closure& task);
        virtual bool PostNonNestableDelayedTask(const base::Closure& task,
            int64 delay_ms);
        virtual bool BelongsToCurrentThread();

    protected:
        // 重载OnDestruct, 这样如果目标消息循环还存在, 我们可以删除其中的对象.
        virtual void OnDestruct();

    private:
        MessageLoopProxyImpl();

        // Called directly by MessageLoop::~MessageLoop.
        virtual void WillDestroyCurrentMessageLoop();

        // TODO(ajwong): Remove this after we've fully migrated to base::Closure.
        bool PostTaskHelper(Task* task, int64 delay_ms, bool nestable);
        bool PostTaskHelper(const base::Closure& task, int64 delay_ms, bool nestable);

        // Allow the messageLoop to create a MessageLoopProxyImpl.
        friend class MessageLoop;

        // target_message_loop_访问保护锁.
        mutable Lock message_loop_lock_;
        MessageLoop* target_message_loop_;

        DISALLOW_COPY_AND_ASSIGN(MessageLoopProxyImpl);
    };

} //namespace base

#endif //__base_message_loop_proxy_impl_h__