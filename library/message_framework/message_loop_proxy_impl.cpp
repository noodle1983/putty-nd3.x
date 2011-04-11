
#include "message_loop_proxy_impl.h"

#include "base/threading/thread_restrictions.h"

namespace base
{

    MessageLoopProxyImpl::MessageLoopProxyImpl()
        : target_message_loop_(MessageLoop::current())
    {
        target_message_loop_->AddDestructionObserver(this);
    }

    MessageLoopProxyImpl::~MessageLoopProxyImpl()
    {
        AutoLock lock(message_loop_lock_);
        // 如果目标消息循环还存在, 析构函数在循环线程中执行.
        if(target_message_loop_)
        {
            DCHECK(MessageLoop::current() == target_message_loop_);
            MessageLoop::current()->RemoveDestructionObserver(this);
        }
    }

    bool MessageLoopProxyImpl::PostTask(Task* task)
    {
        return PostTaskHelper(task, 0, true);
    }

    bool MessageLoopProxyImpl::PostDelayedTask(Task* task, int64 delay_ms)
    {
        return PostTaskHelper(task, delay_ms, true);
    }

    bool MessageLoopProxyImpl::PostNonNestableTask(Task* task)
    {
        return PostTaskHelper(task, 0, false);
    }

    bool MessageLoopProxyImpl::PostNonNestableDelayedTask(Task* task,
        int64 delay_ms)
    {
        return PostTaskHelper(task, delay_ms, false);
    }

    bool MessageLoopProxyImpl::BelongsToCurrentThread()
    {
        // 不应该使用MessageLoop::current(), 因为它使用了LazyInstance, 当WorkerPool
        // 线程调用这个函数的时候可能已被~AtExitManager删除.
        // http://crbug.com/63678
        base::ThreadRestrictions::ScopedAllowSingleton allow_singleton;
        AutoLock lock(message_loop_lock_);
        return (target_message_loop_ &&
            (MessageLoop::current()==target_message_loop_));
    }

    bool MessageLoopProxyImpl::PostTaskHelper(Task* task, int64 delay_ms,
        bool nestable)
    {
        bool ret = false;
        {
            AutoLock lock(message_loop_lock_);
            if(target_message_loop_)
            {
                if(nestable)
                {
                    target_message_loop_->PostDelayedTask(task, delay_ms);
                }
                else
                {
                    target_message_loop_->PostNonNestableDelayedTask(task,
                        delay_ms);
                }
                ret = true;
            }
        }
        if(!ret)
        {
            delete task;
        }
        return ret;
    }

    void MessageLoopProxyImpl::OnDestruct()
    {
        // 不应该使用MessageLoop::current(), 因为它使用了LazyInstance, 当WorkerPool
        // 线程调用这个函数的时候可能已被~AtExitManager删除.
        // http://crbug.com/63678
        ThreadRestrictions::ScopedAllowSingleton allow_singleton;
        bool delete_later = false;
        {
            AutoLock lock(message_loop_lock_);
            if(target_message_loop_ &&
                (MessageLoop::current()!=target_message_loop_))
            {
                target_message_loop_->DeleteSoon(this);
                delete_later = true;
            }
        }
        if(!delete_later)
        {
            delete this;
        }
    }

    void MessageLoopProxyImpl::WillDestroyCurrentMessageLoop()
    {
        AutoLock lock(message_loop_lock_);
        target_message_loop_ = NULL;
    }

    scoped_refptr<MessageLoopProxy> MessageLoopProxy::CreateForCurrentThread()
    {
        scoped_refptr<MessageLoopProxy> ret(new MessageLoopProxyImpl());
        return ret;
    }

} //namespace base