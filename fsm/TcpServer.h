#ifndef TCPSERVER_H
#define TCPSERVER_H
#include <event.h>


namespace Processor
{
    class BoostProcessor;
}

namespace Net
{
class IProtocol;
namespace Reactor
{
    class Reactor;
}
namespace Server{

    class TcpServer
    {
    public:
        TcpServer(
            IProtocol* theProtocol,
            Reactor::Reactor* theReactor,
            Processor::BoostProcessor* theProcessor);
        virtual ~TcpServer();

        void addAcceptEvent();
        int start();
        void stop();

        int asynAccept(int theFd, short theEvt);
        void onAccept(int theFd, short theEvt);

    private:
        IProtocol* protocolM;
        Reactor::Reactor* reactorM;
        Processor::BoostProcessor* processorM;

        struct event* acceptEvtM;
        int portM;
        evutil_socket_t fdM;
    };

} /* Server */
} /* Net */

#endif /*TCPSERVER_H*/

