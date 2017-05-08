#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#include "KfifoBuffer.h"

#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <event.h>

struct timeval;
namespace Processor
{
    typedef boost::function<void ()> Job;
    class BoostProcessor;
}

namespace Net{

class IProtocol;
namespace Reactor
{
    class Reactor;
}
namespace Client
{
    class TcpClient;
}

namespace Connection
{

    class SocketConnection;
    typedef boost::shared_ptr<SocketConnection> SocketConnectionPtr;
    typedef boost::weak_ptr<SocketConnection> SocketConnectionWPtr;
    typedef boost::function<void (int, SocketConnectionPtr)> Watcher;

    class SocketConnection
    {
    public:
        SocketConnection(
            IProtocol* theProtocol,
            Reactor::Reactor* theReactor,
            Processor::BoostProcessor* theProcessor,
            evutil_socket_t theFd);
        ~SocketConnection();

        /**
         * if there is a client, SocketConnection will notify it when connected and error.
         */
        SocketConnection(
            IProtocol* theProtocol,
            Reactor::Reactor* theReactor,
            Processor::BoostProcessor* theProcessor,
            evutil_socket_t theFd,
            Client::TcpClient* theClient);

        //interface for reactor
        int asynRead(int theFd, short theEvt);
        int asynWrite(int theFd, short theEvt);

        //interface for upper protocol
        void addClientTimer(unsigned theSec);
        static void onClientTimeout(int theFd, short theEvt, void *theArg);

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

        void setLowWaterMarkWatcher(Watcher* theWatcher);

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
        static void onHeartbeat(int theFd, short theEvt, void *theArg);

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
        struct event* heartbeatTimerEvtM;
        struct event* clientTimerEvtM;
        int heartbeatTimeoutCounterM;

        IProtocol* protocolM;
        Reactor::Reactor* reactorM;
        Processor::BoostProcessor* processorM;

        evutil_socket_t fdM;

        //we ensure there is only 1 thread read/write the input queue
        //boost::mutex inputQueueMutexM;
        Utility::KfifoBuffer inputQueueM;
        boost::mutex outputQueueMutexM;
        Utility::KfifoBuffer outputQueueM;

        enum Status{ActiveE = 0, CloseE = 1};
        mutable int statusM;

        boost::mutex stopReadingMutexM;
        bool stopReadingM;
        boost::mutex watcherMutexM;
        Watcher* watcherM;

        Client::TcpClient* clientM;
        boost::mutex clientMutexM;
        bool isConnectedNotified;

		struct sockaddr_in peerAddrM;
		struct sockaddr_in localAddrM;
        
        //upper data
        void* uppperDataM;
    };

}
}

#endif /*SOCKETCONNECTION_H*/

