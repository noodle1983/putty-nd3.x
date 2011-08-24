
#include "message_loop_proxy_impl.h"

#include "threading/thread_restrictions.h"

namespace base
{

    MessageLoopProxyImpl::MessageLoopProxyImpl()
        : target_message_loop_(MessageLoop::current()) {}

    MessageLoopProxyImpl::~MessageLoopProxyImpl() {}

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

    bool MessageLoopProxyImpl::PostTask(const base::Closure& task)
    {
        return PostTaskHelper(task, 0, true);
    }

    bool MessageLoopProxyImpl::PostDelayedTask(const base::Closure& task,
        int64 delay_ms)
    {
        return PostTaskHelper(task, delay_ms, true);
    }

    bool MessageLoopProxyImpl::PostNonNestableTask(const base::Closure& task)
    {
        return PostTaskHelper(task, 0, false);
    }

    bool MessageLoopProxyImpl::PostNonNestableDelayedTask(
        const base::Closure& task, int64 delay_ms)
    {
        return PostTaskHelper(task, delay_ms, false);
    }

    bool MessageLoopProxyImpl::BelongsToCurrentThread()
    {
        // 不应该使用MessageLoop::current(), 因为它使用了LazyInstance, 当WorkerPool
        // 线程调用这个函数的时候可能已被~AtExitManager删除.
        // http://crbug.com/63678
        ThreadRestrictions::ScopedAllowSingleton allow_singleton;
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
                    target_message_loop_->PostNonNestableDelayedTask(
                        task, delay_ms);
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

    bool MessageLoopProxyImpl::PostTaskHelper(const base::Closure& task,
        int64 delay_ms, bool nestable)
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
                target_message_loop_->PostNonNestableDelayedTask(task, delay_ms);
            }
            return true;
        }
        return false;
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

    scoped_refptr<MessageLoopProxy> MessageLoopProxy::current()
    {
        MessageLoop* cur_loop = MessageLoop::current();
        if(!cur_loop)
        {
            return NULL;
        }
        return cur_loop->message_loop_proxy();
    }

} //namespace base