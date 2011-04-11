
#include "message_loop_proxy.h"

namespace base
{

    MessageLoopProxy::MessageLoopProxy() {}

    MessageLoopProxy::~MessageLoopProxy() {}

    void MessageLoopProxy::OnDestruct()
    {
        delete this;
    }

} //namespace base