#ifndef TCPSERVER_H
#define TCPSERVER_H
#include "../libevent/include/event.h"


namespace Processor
{
    class WinProcessor;
}

namespace Net
{
class IProtocol;
class Reactor;

class TcpServer
{
public:
    TcpServer(
        IProtocol* theProtocol);
    virtual ~TcpServer();

    void addAcceptEvent();
    int start();
    void stop();

    int asynAccept(int theFd, short theEvt);
    void onAccept(int theFd, short theEvt);

private:
    IProtocol* protocolM;

    struct event* acceptEvtM;
    int portM;
    evutil_socket_t fdM;
};
} /* Net */

#endif /*TCPSERVER_H*/

