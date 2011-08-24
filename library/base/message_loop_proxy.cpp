
#include "message_loop_proxy.h"

#include "bind.h"

namespace base
{

    namespace
    {

        // This relay class remembers the MessageLoop that it was created on, and
        // ensures that both the |task| and |reply| Closures are deleted on this same
        // thread. Also, |task| is guaranteed to be deleted before |reply| is run or
        // deleted.
        //
        // If this is not possible because the originating MessageLoop is no longer
        // available, the the |task| and |reply| Closures are leaked.  Leaking is
        // considered preferable to having a thread-safetey violations caused by
        // invoking the Closure destructor on the wrong thread.
        class PostTaskAndReplyRelay
        {
        public:
            PostTaskAndReplyRelay(const Closure& task, const Closure& reply)
                : origin_loop_(MessageLoopProxy::current())
            {
                task_ = task;
                reply_ = reply;
            }

            ~PostTaskAndReplyRelay()
            {
                DCHECK(origin_loop_->BelongsToCurrentThread());
                task_.Reset();
                reply_.Reset();
            }

            void Run()
            {
                task_.Run();
                origin_loop_->PostTask(
                    Bind(&PostTaskAndReplyRelay::RunReplyAndSelfDestruct,
                    base::Unretained(this)));
            }

        private:
            void RunReplyAndSelfDestruct()
            {
                DCHECK(origin_loop_->BelongsToCurrentThread());

                // Force |task_| to be released before |reply_| is to ensure that no one
                // accidentally depends on |task_| keeping one of its arguments alive while
                // |reply_| is executing.
                task_.Reset();

                reply_.Run();

                // Cue mission impossible theme.
                delete this;
            }

            scoped_refptr<MessageLoopProxy> origin_loop_;
            Closure reply_;
            Closure task_;
        };

    } //namespace

    MessageLoopProxy::MessageLoopProxy() {}

    MessageLoopProxy::~MessageLoopProxy() {}

    bool MessageLoopProxy::PostTaskAndReply(const Closure& task, const Closure& reply)
    {
        PostTaskAndReplyRelay* relay = new PostTaskAndReplyRelay(task, reply);
        if(!PostTask(Bind(&PostTaskAndReplyRelay::Run, Unretained(relay))))
        {
            delete relay;
            return false;
        }

        return true;
    }

    void MessageLoopProxy::OnDestruct() const
    {
        delete this;
    }

} //namespace base