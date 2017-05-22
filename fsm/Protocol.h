#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "WinProcessor.h"

struct event;

namespace Net
{
	class SocketConnection;
	typedef std::shared_ptr<SocketConnection> SocketConnectionPtr;
    class IProtocol
    {
    public:
		IProtocol()
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
        int asynHandleInput(const int theFd, SocketConnectionPtr theConnection)
        {
			return g_ui_processor->process(0, NEW_PROCESSOR_JOB(
                    &IProtocol::handleInput, this, theConnection));
        }
        int asynHandleClose(const int theFd, SocketConnectionPtr theConnection)
        {
			return g_ui_processor->process(0, NEW_PROCESSOR_JOB(
                    &IProtocol::handleClose, this, theConnection));
        }
        int asynHandleConnected(const int theFd, SocketConnectionPtr theConnection)
        {
			return g_ui_processor->process(0, NEW_PROCESSOR_JOB(
                    &IProtocol::handleConnected, this, theConnection));
        }
        int asynHandleHeartbeat(const int theFd, SocketConnectionPtr theConnection) 
        {
			return g_ui_processor->process(0, NEW_PROCESSOR_JOB(
                    &IProtocol::handleHeartbeat, this, theConnection));
        }
		inline struct min_heap_item_t* addLocalTimer(
				const int theFd,
				const struct timeval& theInterval, 
				Processor::TimeoutFn theCallback,
				void* theArg)
        {
			return g_ui_processor->addLocalTimer(
					theInterval, theCallback, theArg);
        }
		inline void cancelLocalTimer(
                const int theFd, 
				struct min_heap_item_t*& theEvent)
        {
			return g_ui_processor->cancelLocalTimer(theEvent);
		}
        
        virtual void handleInput(Net::SocketConnectionPtr theConnection) = 0;
        virtual void handleClose(Net::SocketConnectionPtr theConnection) {}
        virtual void handleConnected(SocketConnectionPtr theConnection) {}
        /*
         * send heartbeat msg in heartbeat 
         * or close the Connection if the connection is no response.
         */
        virtual void handleHeartbeat(SocketConnectionPtr theConnection) {}

        //Config
        virtual const std::string getAddr(){ return "127.0.0.1"; }
        virtual int getPort(){ return 0; }
        virtual int getRBufferSizePower(){ return 20; }
        virtual int getWBufferSizePower(){ return 20; }
        virtual int getHeartbeatInterval(){ return 0; }
        virtual int getMaxHeartbeatTimeout(){ return 3; }
        
    private:
    };
}

#endif /*PROTOCOL_H*/

