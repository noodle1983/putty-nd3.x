#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "BoostProcessor.h"
#include <boost/bind.hpp>

struct event;

namespace Net
{
    namespace Connection
    {
        class SocketConnection;
        typedef boost::shared_ptr<SocketConnection> SocketConnectionPtr;
    }
    namespace Server
    {
        class UdpServer;
        typedef boost::shared_ptr<UdpServer> UdpServerPtr;
    }
    class IProtocol
    {
    public:
        IProtocol(Processor::BoostProcessor* theProcessor)
            : processorM(theProcessor)
        {
        }
        virtual ~IProtocol() {};

        /**
         *
         * interface: asynHandleInput
         * Description: the net framework will notify the protocol object the input event,
         *         For the performance, Protocol should handle the input in another thread.
         * the Args:
         *         theFd: which socket the input is from
         *         connection: the socket connection which can be write to
         *
         */
        int asynHandleInput(const int theFd, Connection::SocketConnectionPtr theConnection)
        {
            return processorM->process(theFd + 1,
                    &IProtocol::handleInput, this, theConnection);
        }
        int asynHandleClose(const int theFd, Connection::SocketConnectionPtr theConnection)
        {
            return processorM->process(theFd + 1,
                    &IProtocol::handleClose, this, theConnection);
        }
        int asynHandleConnected(const int theFd, Connection::SocketConnectionPtr theConnection)
        {
            return processorM->process(theFd + 1,
                    &IProtocol::handleConnected, this, theConnection);
        }
        int asynHandleHeartbeat(const int theFd, Connection::SocketConnectionPtr theConnection) 
        {
            return processorM->process(theFd + 1,
                    &IProtocol::handleHeartbeat, this, theConnection);
        }
		inline struct event* addLocalTimer(
				const int theFd,
				const struct timeval& theInterval, 
				event_callback_fn theCallback,
				void* theArg)
        {
			return processorM->addLocalTimer(theFd + 1, 
					theInterval, theCallback, theArg);
        }
		inline void cancelLocalTimer(
                const int theFd, 
                struct event*& theEvent)
        {
			return processorM->cancelLocalTimer(theFd + 1,theEvent);
		}
        
        virtual void handleInput(Net::Connection::SocketConnectionPtr theConnection) = 0;
        virtual void handleClose(Net::Connection::SocketConnectionPtr theConnection) {}
        virtual void handleConnected(Connection::SocketConnectionPtr theConnection) {}
        /*
         * send heartbeat msg in heartbeat 
         * or close the Connection if the connection is no response.
         */
        virtual void handleHeartbeat(Connection::SocketConnectionPtr theConnection) {}

        //Config
        virtual const std::string getAddr(){ return "0.0.0.0"; }
        virtual int getPort(){ return 5460; }
        virtual int getRBufferSizePower(){ return 20; }
        virtual int getWBufferSizePower(){ return 20; }
        virtual int getHeartbeatInterval(){ return 0; }
        virtual int getMaxHeartbeatTimeout(){ return 3; }
        
    private:
        Processor::BoostProcessor* processorM;
    };

    class IClientProtocol: public IProtocol
    {
    public:
        IClientProtocol(Processor::BoostProcessor* theProcessor)
            :IProtocol(theProcessor)
        {
        }
        virtual ~IClientProtocol() {};

        //Config
        virtual unsigned getReConnectInterval(){ return 5; }
        virtual const std::string getAddr(){ return "127.0.0.1"; }
    };

    class IUdpProtocol
    {
    public:
        IUdpProtocol(Processor::BoostProcessor* theProcessor)
            : processorM(theProcessor)
        {
        }
        virtual ~IUdpProtocol() {};

        /**
         *
         * interface: asynHandleInput
         * Description: the net framework will notify the protocol object the input event,
         *         For the performance, Protocol should handle the input in another thread.
         * the Args:
         *         theFd: which socket the input is from
         *         connection: the socket connection which can be write to
         *
         */
        int asynHandleInput(int theFd, Server::UdpServerPtr theUdpServer)
        {
            return processorM->process(theFd,
                    &IUdpProtocol::handleInput, this, theUdpServer);
        }
        
        virtual void handleInput(Net::Server::UdpServerPtr theUdpServer) = 0;
        
        //Config
        virtual int getRBufferSizePower(){ return 20; }
    private:
        Processor::BoostProcessor* processorM;
    };

}

#endif /*PROTOCOL_H*/

