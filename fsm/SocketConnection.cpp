#include "BoostProcessor.h"
#include "SocketConnection.h"
#include "TcpClient.h"
#include "Reactor.h"
#include "Protocol.h"
#include "Log.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <err.h>


using namespace Net::Connection;

//-----------------------------------------------------------------------------

void on_read(int theFd, short theEvt, void *theArg)
{
    SocketConnection* connection = (SocketConnection*)theArg;
    connection->asynRead(theFd, theEvt);
}

//-----------------------------------------------------------------------------

void on_write(int theFd, short theEvt, void *theArg)
{
    SocketConnection* connection = (SocketConnection*)theArg;
    connection->asynWrite(theFd, theEvt);
}

//-----------------------------------------------------------------------------

SocketConnection::SocketConnection(
            IProtocol* theProtocol,
            Reactor::Reactor* theReactor,
            Processor::BoostProcessor* theProcessor,
            evutil_socket_t theFd)
    : selfM(this)
    , heartbeatTimerEvtM(NULL)
    , clientTimerEvtM(NULL)
    , heartbeatTimeoutCounterM(0)
    , protocolM(theProtocol)
    , reactorM(theReactor)
    , processorM(theProcessor)
    , fdM(theFd)
    , inputQueueM(theProtocol->getRBufferSizePower())
    , outputQueueM(theProtocol->getWBufferSizePower())
    , statusM(ActiveE)
    , stopReadingM(false)
    , watcherM(NULL)
    , clientM(NULL)
    , isConnectedNotified(true)
    , uppperDataM(NULL)
{
    readEvtM = reactorM->newEvent(fdM, EV_READ, on_read, this);
    writeEvtM = reactorM->newEvent(fdM, EV_WRITE, on_write, this);
    addReadEvent();
    protocolM->asynHandleConnected(fdM, selfM);
    processorM->process(fdM, &SocketConnection::startHeartbeatTimer, this);
}

//-----------------------------------------------------------------------------

SocketConnection::SocketConnection(
            IProtocol* theProtocol,
            Reactor::Reactor* theReactor,
            Processor::BoostProcessor* theProcessor,
            evutil_socket_t theFd,
            Client::TcpClient* theClient)
    : selfM(this)
    , heartbeatTimerEvtM(NULL)
    , clientTimerEvtM(NULL)
    , heartbeatTimeoutCounterM(0)
    , protocolM(theProtocol)
    , reactorM(theReactor)
    , processorM(theProcessor)
    , fdM(theFd)
    , inputQueueM(theProtocol->getRBufferSizePower())
    , outputQueueM(theProtocol->getWBufferSizePower())
    , statusM(ActiveE)
    , stopReadingM(false)
    , watcherM(NULL)
    , clientM(theClient)
    , isConnectedNotified(false)
    , uppperDataM(NULL)
{
    readEvtM = reactorM->newEvent(fdM, EV_READ, on_read, this);
    writeEvtM = reactorM->newEvent(fdM, EV_WRITE, on_write, this);
    addWriteEvent();
    addReadEvent();
}
//-----------------------------------------------------------------------------

SocketConnection::~SocketConnection()
{
    evutil_closesocket(fdM);
    if (watcherM)
    {
        delete watcherM;
        watcherM = NULL;
    }
    if (uppperDataM != NULL)
    {
        LOG_WARN("uppperDataM is not NULL and may leek, please free it and set to NULL in upper handling while connection is closed.");
    }
    LOG_DEBUG("close fd:" << fdM);
}

//-----------------------------------------------------------------------------

void SocketConnection::rmClient()
{
    boost::lock_guard<boost::mutex> lock(clientMutexM);
    clientM = NULL;
}

//-----------------------------------------------------------------------------

void SocketConnection::addReadEvent()
{
    if (CloseE == statusM)
        return;
    if (-1 == event_add(readEvtM, NULL))
    {
        processorM->process(fdM, &SocketConnection::addReadEvent, this);
    }
}

//-----------------------------------------------------------------------------

void SocketConnection::addWriteEvent()
{
    if (CloseE == statusM)
        return;
    if (-1 == event_add(writeEvtM, NULL))
    {
        processorM->process(fdM, &SocketConnection::addWriteEvent, this);
    }
}

//-----------------------------------------------------------------------------

int SocketConnection::asynRead(int theFd, short theEvt)
{
    return processorM->process(fdM, &SocketConnection::onRead, this, theFd, theEvt);
}

//-----------------------------------------------------------------------------

void SocketConnection::onRead(int theFd, short theEvt)
{
    char buffer[1024]= {0};
    unsigned readBufferLeft = inputQueueM.unusedSize();
    unsigned readLen = (readBufferLeft < sizeof(buffer)) ? readBufferLeft : sizeof(buffer);
    if (readLen == 0)
    {
        if (!stopReadingM)
        {
            boost::lock_guard<boost::mutex> lock(stopReadingMutexM);
            stopReadingM = true;
        }
        protocolM->asynHandleInput(fdM, selfM);
        return;
    }

    int len = read(theFd, buffer, readLen);
    if ((0 == len) ||(len < 0 && errno != EWOULDBLOCK))
    {
        LOG_DEBUG("Client disconnected. fd:" << fdM);
        _close();
        return;
    }
    else if (len > SSIZE_MAX)
    {
        LOG_WARN("Socket failure, disconnecting client:" << strerror(errno));
        _close();
        return;
    }
    else if (len < 0 && errno == EWOULDBLOCK)
    {
        len = 0;
    }
    unsigned putLen = inputQueueM.put(buffer, len);
    assert(putLen == (unsigned)len);

    while(len > 0 && (Utility::BufferOkE == inputQueueM.getStatus()
                    || Utility::BufferLowE == inputQueueM.getStatus() ))
    {
        readBufferLeft = inputQueueM.unusedSize();
        readLen = (readBufferLeft < sizeof(buffer)) ? readBufferLeft : sizeof(buffer);
        len = read(theFd, buffer, readLen);
        if (len <= 0 || len > SSIZE_MAX)
        {
            break;
        }
        putLen = inputQueueM.put(buffer, len);
        assert(putLen == (unsigned)len);
    }

    if (Utility::BufferHighE == inputQueueM.getStatus()
            || Utility::BufferNotEnoughE == inputQueueM.getStatus())
    {
        //TRACE("Flow Control:Socket " << fdM << " stop reading.", fdM);
        boost::lock_guard<boost::mutex> lock(stopReadingMutexM);
        stopReadingM = true;
    }
    else
    {
        addReadEvent();
    }
    protocolM->asynHandleInput(fdM, selfM);
}

//-----------------------------------------------------------------------------

unsigned SocketConnection::getInput(char* const theBuffer, const unsigned theLen)
{
    //if (CloseE == statusM)
    //    return 0;

    unsigned len = inputQueueM.get(theBuffer, theLen);
    if (stopReadingM && CloseE != statusM)
    {
        Utility::BufferStatus postBufferStatus = inputQueueM.getStatus();
        if (postBufferStatus == Utility::BufferLowE)
        {
            {
                boost::lock_guard<boost::mutex> lock(stopReadingMutexM);
                stopReadingM = false;
            }
            asynRead(fdM, 0);
            //processorM->process(fdM, &SocketConnection::addReadEvent, selfM);
        }
    }
    return len;
}

//-----------------------------------------------------------------------------

unsigned SocketConnection::getnInput(char* const theBuffer, const unsigned theLen)
{
    //if (CloseE == statusM)
    //    return 0;

    unsigned len = inputQueueM.getn(theBuffer, theLen);
    if (stopReadingM && CloseE != statusM)
    {
        Utility::BufferStatus postBufferStatus = inputQueueM.getStatus();
        if (postBufferStatus == Utility::BufferLowE)
        {
            {
                boost::lock_guard<boost::mutex> lock(stopReadingMutexM);
                stopReadingM = false;
            }
            asynRead(fdM, 0);
            //processorM->process(fdM, &SocketConnection::addReadEvent, selfM);
        }
    }
    return len;
}

//-----------------------------------------------------------------------------

unsigned SocketConnection::peeknInput(char* const theBuffer, const unsigned theLen)
{
    //if (CloseE == statusM)
    //    return 0;

    return inputQueueM.peekn(theBuffer, theLen);
}

//-----------------------------------------------------------------------------

unsigned SocketConnection::sendn(const char* const theBuffer, const unsigned theLen)
{
    if (CloseE == statusM)
        return 0;

    if (0 == theLen || NULL == theBuffer)
        return 0;

    unsigned len = 0;
    {
        boost::lock_guard<boost::mutex> lock(outputQueueMutexM);
        len = outputQueueM.putn(theBuffer, theLen);
    }
    if (0 == len)
    {
        LOG_WARN("outage of the connection's write queue!");
    }
    processorM->process(fdM, &SocketConnection::addWriteEvent, selfM);
    return len;
}

//-----------------------------------------------------------------------------

int SocketConnection::asynWrite(int theFd, short theEvt)
{
    if (CloseE == statusM)
        return -1;
    return processorM->process(fdM, &SocketConnection::onWrite, this, theFd, theEvt);
}

//-----------------------------------------------------------------------------

void SocketConnection::setLowWaterMarkWatcher(Watcher* theWatcher)
{
	//if it is already writable
    Utility::BufferStatus bufferStatus = outputQueueM.getStatus();
	if (bufferStatus == Utility::BufferLowE)
	{
		(*theWatcher)(fdM, selfM);
		return;
	}

	//or set theWatcher and add the write event
	{
		boost::lock_guard<boost::mutex> lock(watcherMutexM);
		if (watcherM)
		{
			delete watcherM;
		}
		watcherM = theWatcher;
	}
    if (CloseE != statusM)
    {
        addWriteEvent();
    }
}

//-----------------------------------------------------------------------------

void SocketConnection::onWrite(int theFd, short theEvt)
{
    if (!isConnectedNotified && clientM)
    {
        boost::lock_guard<boost::mutex> lock(clientMutexM);
        if (clientM)
        {
            clientM->onConnected(theFd, selfM);
            isConnectedNotified = true;
            startHeartbeatTimer();
        }
    }
    char buffer[1024]= {0};
    unsigned peekLen = outputQueueM.peek(buffer, sizeof(buffer));
    int writeLen = 0;
    while (CloseE != statusM && peekLen > 0)
    {
        writeLen = write(theFd, buffer, peekLen);
        if (writeLen < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                break;
            }
            else
            {
                LOG_DEBUG("Socket write failure.");
                _close();
                return;
            }
        }
        outputQueueM.commitRead(writeLen);
        peekLen = outputQueueM.peek(buffer, sizeof(buffer));
    }

    Utility::BufferStatus bufferStatus = outputQueueM.getStatus();
    if (watcherM && (bufferStatus == Utility::BufferLowE))
    {
        boost::lock_guard<boost::mutex> lock(watcherMutexM);
        if (watcherM)
        {
            (*watcherM)(fdM, selfM);
            delete watcherM;
            watcherM = NULL;
        }
    }

    if (CloseE != statusM && !outputQueueM.empty())
    {
        addWriteEvent();
    }

}

//-----------------------------------------------------------------------------

void SocketConnection::startHeartbeatTimer()
{
    struct timeval tv;
    tv.tv_sec = protocolM->getHeartbeatInterval(); 
    if (tv.tv_sec <= 0)
        tv.tv_sec = 60;
    tv.tv_usec = 0;

    heartbeatTimerEvtM = processorM->addLocalTimer(fdM, tv, SocketConnection::onHeartbeat, this);
}

//-----------------------------------------------------------------------------

void SocketConnection::onHeartbeat(int theFd, short theEvt, void *theArg)
{
    SocketConnection* connection = (SocketConnection*) theArg;
    connection->heartbeatTimerEvtM = NULL;

    int heartbeatInterval = connection->protocolM->getHeartbeatInterval();
    if (heartbeatInterval > 0)
    {
        connection->protocolM->asynHandleHeartbeat(connection->fdM, connection->selfM);
    }

    connection->startHeartbeatTimer();
}

//-----------------------------------------------------------------------------

void SocketConnection::addClientTimer(unsigned theSec)
{
    processorM->process(fdM, &SocketConnection::_addClientTimer, this, theSec);
}

//-----------------------------------------------------------------------------

void SocketConnection::_addClientTimer(unsigned theSec)
{
    if (NULL == clientM || 0 == theSec)
    {
        return;
    }
    if (clientTimerEvtM)
    {
        processorM->cancelLocalTimer(fdM, clientTimerEvtM);
    }

    struct timeval tv;
    tv.tv_sec = theSec; 
    tv.tv_usec = 0;

    clientTimerEvtM = processorM->addLocalTimer(fdM, tv, &SocketConnection::onClientTimeout, this);

}

//-----------------------------------------------------------------------------

void SocketConnection::onClientTimeout(int theFd, short theEvt, void *theArg)
{
    SocketConnection* connection = (SocketConnection*) theArg;
    boost::lock_guard<boost::mutex> lock(connection->clientMutexM);
    connection->clientTimerEvtM = NULL;
    if (connection->clientM)
    {
        connection->clientM->onClientTimeout();
    }

}

//-----------------------------------------------------------------------------

void SocketConnection::close()
{
    if (CloseE == statusM)
        return;
    processorM->process(fdM, &SocketConnection::_close, this);
}

//-----------------------------------------------------------------------------

void SocketConnection::_release()
{
    selfM.reset();
}

//-----------------------------------------------------------------------------

void SocketConnection::_close()
{
    if (CloseE == statusM)
        return;
    statusM = CloseE;

    if (heartbeatTimerEvtM)
    {
        processorM->cancelLocalTimer(fdM, heartbeatTimerEvtM);
    }
    if (clientTimerEvtM)
    {
        processorM->cancelLocalTimer(fdM, clientTimerEvtM);
    }
	protocolM->asynHandleClose(fdM, selfM);
    if (clientM)
    {
        boost::lock_guard<boost::mutex> lock(clientMutexM);
        if (clientM)
        {
            clientM->onError();
            clientM = NULL;
        }
    }
    reactorM->delEvent(readEvtM);
    reactorM->delEvent(writeEvtM);
    processorM->process(fdM, &SocketConnection::_release, this);
}

//-----------------------------------------------------------------------------


