#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#include "boost/function.hpp"
#include "boost/bind.hpp"
#include "min_heap.h"
#include "KfifoBuffer.h"
#include "WinMutex.h"
#include "../libevent/include/event.h"

#include<memory>
struct timeval;
namespace Processor
{
    typedef boost::function<void ()> Job;
}

namespace Net{

class IProtocol;
class Reactor;
class TcpClient;

//class SocketConnection;
//typedef boost::shared_ptr<SocketConnection> SocketConnectionPtr;
//typedef boost::weak_ptr<SocketConnection> SocketConnectionWPtr;

class SocketConnection
{
public:
	typedef std::shared_ptr<SocketConnection> SocketConnectionPtr;
	typedef std::weak_ptr<SocketConnection> SocketConnectionWPtr;
    SocketConnection(
        IProtocol* theProtocol,
        Reactor* theReactor,
        evutil_socket_t theFd);
    ~SocketConnection();

    /**
        * if there is a client, SocketConnection will notify it when connected and error.
        */
//    SocketConnection(
//        IProtocol* theProtocol,
//        Reactor* theReactor,
//        evutil_socket_t theFd,
//        TcpClient* theClient);

    //interface for reactor
    int asynRead(int theFd, short theEvt);
    int asynWrite(int theFd, short theEvt);

    //interface for upper protocol
    void addClientTimer(unsigned theSec);
    static void onClientTimeout(void *theArg);

    void rmClient();
    SocketConnectionPtr self(){return selfM;}
    void close();
    inline bool isClose() {return statusM == CloseE;}
    inline bool isRBufferHealthy(){return inputQueueM.isHealthy();};
    inline bool isWBufferHealthy(){return outputQueueM.isHealthy();};
    unsigned getRBufferSize(){return inputQueueM.size();};
    unsigned getWBufferSize(){return outputQueueM.size();};

    unsigned getInput(char* const theBuffer, const unsigned theLen);
    unsigned getnInput(char* const theBuffer, const unsigned theLen);
    unsigned peeknInput(char* const theBuffer, const unsigned theLen);
    unsigned sendn(const char* const theBuffer, const unsigned theLen);

	//attribute
	int getFd(){return fdM;}
	void setPeerAddr(const struct sockaddr_in* theAddr){peerAddrM = *theAddr;}
	const struct sockaddr_in& getPeerAddr(){return peerAddrM;}

    //heartbeatTimeoutCounterM
    void resetHeartbeatTimeoutCounter(){heartbeatTimeoutCounterM = 0;}
    int incHeartbeatTimeoutCounter(){return heartbeatTimeoutCounterM++;}
    int getHeartbeatTimeoutCounter(){return heartbeatTimeoutCounterM;}

    void* getUpperData(){return uppperDataM;}
    void setUpperData(void* theUpperData){uppperDataM = theUpperData;}

private:
    friend class boost::function<void ()>;
    void addReadEvent();
    void addWriteEvent();
    void onRead(int theFd, short theEvt);
    void onWrite(int theFd, short theEvt);

    void _addClientTimer(unsigned theSec);

    void startHeartbeatTimer();
    static void onHeartbeat(void *theArg);

    void _close();
    void _release();


private:
    //for reactor:
    //  this is enough
    //  we ensure after delete there is no msg from the libevent
    //for uppper protocol:
    //  selfM should be applied
    //  in case there is a msg after connection is delete
    SocketConnectionPtr selfM;

    struct event* readEvtM;
    struct event* writeEvtM;
	struct min_heap_item_t* heartbeatTimerEvtM;
	struct min_heap_item_t* clientTimerEvtM;
    int heartbeatTimeoutCounterM;
	 
    IProtocol* protocolM;
    Reactor* reactorM;

    evutil_socket_t fdM;

    //we ensure there is only 1 thread read/write the input queue
    //boost::mutex inputQueueMutexM;
    KfifoBuffer inputQueueM;
    Lock outputQueueMutexM;
    KfifoBuffer outputQueueM;

    enum Status{ActiveE = 0, CloseE = 1};
    mutable int statusM;

    Lock stopReadingMutexM;
    bool stopReadingM;
    Lock watcherMutexM;

    TcpClient* clientM;
    Lock clientMutexM;
    bool isConnectedNotified;

	struct sockaddr_in peerAddrM;
	struct sockaddr_in localAddrM;
        
    //upper data
    void* uppperDataM;
};

}

#endif /*SOCKETCONNECTION_H*/

