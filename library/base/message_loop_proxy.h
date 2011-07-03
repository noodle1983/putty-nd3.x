
#ifndef __base_message_loop_proxy_h__
#define __base_message_loop_proxy_h__

#pragma once

#include "task.h"

namespace base
{

    struct MessageLoopProxyTraits;

    // MessageLoopProxy为消息循环的Post*方法提供一个线程安全的引用计数接口, 类对象
    // 比目标消息循环对象的生命周期要长. 可以通过Thread::message_loop_proxy()或者
    // MessageLoopProxy::CreateForCurrentThread()获取MessageLoopProxy.
    class MessageLoopProxy : public RefCountedThreadSafe<MessageLoopProxy,
        MessageLoopProxyTraits>
    {
    public:
        // 这些方法和message_loop.h中的一样, 但是不保证每个任务都可以投递到MessageLoop
        // (如果对象还在的话是可以保证的), 如果没有投递成功会删除任务.
        // 如果线程存在且任务投递成功, 函数返回true. 注意即使任务投递成功, 也不能保证
        // 被执行, 因为目标线程的消息队列中可能已经存在一个Quit消息了.
        virtual bool PostTask(Task* task) = 0;
        virtual bool PostDelayedTask(Task* task, int64 delay_ms) = 0;
        virtual bool PostNonNestableTask(Task* task) = 0;
        virtual bool PostNonNestableDelayedTask(Task* task, int64 delay_ms) = 0;
        // 检查调用者当前是否在proxy代表的线程中执行.
        virtual bool BelongsToCurrentThread() = 0;

        template<class T>
        bool DeleteSoon(T* object)
        {
            return PostNonNestableTask(new DeleteTask<T>(object));
        }
        template<class T>
        bool ReleaseSoon(T* object)
        {
            return PostNonNestableTask(new ReleaseTask<T>(object));
        }

        // 为当前线程创建MessageLoopProxy对象的工厂方法.
        static scoped_refptr<MessageLoopProxy> CreateForCurrentThread();

    protected:
        friend class RefCountedThreadSafe<MessageLoopProxy, MessageLoopProxyTraits>;
        friend struct MessageLoopProxyTraits;

        MessageLoopProxy();
        virtual ~MessageLoopProxy();

        // 在proxy即将被删除的时候调用. 派生类可以重载这个函数, 实现特定线程的清理.
        virtual void OnDestruct();
    };

    struct MessageLoopProxyTraits
    {
        static void Destruct(MessageLoopProxy* proxy)
        {
            proxy->OnDestruct();
        }
    };

} //namespace base

#endif //__base_message_loop_proxy_h__