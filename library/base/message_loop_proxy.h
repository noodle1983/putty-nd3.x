
#ifndef __base_message_loop_proxy_h__
#define __base_message_loop_proxy_h__

#pragma once

#include "callback.h"
#include "task.h"

namespace base
{

    struct MessageLoopProxyTraits;

    // This class provides a thread-safe refcounted interface to the Post* methods
    // of a message loop. This class can outlive the target message loop.
    // MessageLoopProxy objects are constructed automatically for all MessageLoops.
    // So, to access them, you can use any of the following:
    //   Thread::message_loop_proxy()
    //   MessageLoop::current()->message_loop_proxy()
    //   MessageLoopProxy::current()
    class MessageLoopProxy
        : public base::RefCountedThreadSafe<MessageLoopProxy, MessageLoopProxyTraits>
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

        // TODO(ajwong): Remove the functions above once the Task -> Closure migration
        // is complete.
        //
        // There are 2 sets of Post*Task functions, one which takes the older Task*
        // function object representation, and one that takes the newer base::Closure.
        // We have this overload to allow a staged transition between the two systems.
        // Once the transition is done, the functions above should be deleted.
        virtual bool PostTask(const base::Closure& task) = 0;
        virtual bool PostDelayedTask(const base::Closure& task, int64 delay_ms) = 0;
        virtual bool PostNonNestableTask(const base::Closure& task) = 0;
        virtual bool PostNonNestableDelayedTask(const base::Closure& task,
            int64 delay_ms) = 0;

        // 检查调用者当前是否在proxy代表的线程中执行.
        virtual bool BelongsToCurrentThread() = 0;

        // Executes |task| on the given MessageLoopProxy.  On completion, |reply|
        // is passed back to the MessageLoopProxy for the thread that called
        // PostTaskAndReply().  Both |task| and |reply| are guaranteed to be deleted
        // on the thread from which PostTaskAndReply() is invoked.  This allows
        // objects that must be deleted on the originating thread to be bound into the
        // |task| and |reply| Closures.  In particular, it can be useful to use
        // WeakPtr<> in the |reply| Closure so that the reply operation can be
        // canceled. See the following pseudo-code:
        //
        // class DataBuffer : public RefCountedThreadSafe<DataBuffer> {
        //  public:
        //   // Called to add data into a buffer.
        //   void AddData(void* buf, size_t length);
        //   ...
        // };
        //
        //
        // class DataLoader : public SupportsWeakPtr<ReadToBuffer> {
        //  public:
        //    void GetData() {
        //      scoped_refptr<DataBuffer> buffer = new DataBuffer();
        //      target_thread_.message_loop_proxy()->PostTaskAndReply(
        //          FROM_HERE,
        //          base::Bind(&DataBuffer::AddData, buffer),
        //          base::Bind(&DataLoader::OnDataReceived, AsWeakPtr(), buffer));
        //    }
        //
        //  private:
        //    void OnDataReceived(scoped_refptr<DataBuffer> buffer) {
        //      // Do something with buffer.
        //    }
        // };
        //
        //
        // Things to notice:
        //   * Results of |task| are shared with |reply| by binding a shared argument
        //     (a DataBuffer instance).
        //   * The DataLoader object has no special thread safety.
        //   * The DataLoader object can be deleted while |task| is still running,
        //     and the reply will cancel itself safely because it is bound to a
        //     WeakPtr<>.
        bool PostTaskAndReply(const Closure& task, const Closure& reply);

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

        // Gets the MessageLoopProxy for the current message loop, creating one if
        // needed.
        static scoped_refptr<MessageLoopProxy> current();

    protected:
        friend class RefCountedThreadSafe<MessageLoopProxy, MessageLoopProxyTraits>;
        friend struct MessageLoopProxyTraits;

        MessageLoopProxy();
        virtual ~MessageLoopProxy();

        // 在proxy即将被删除的时候调用. 派生类可以重载这个函数, 实现特定线程的清理.
        virtual void OnDestruct() const;
    };

    struct MessageLoopProxyTraits
    {
        static void Destruct(const MessageLoopProxy* proxy)
        {
            proxy->OnDestruct();
        }
    };

} //namespace base

#endif //__base_message_loop_proxy_h__