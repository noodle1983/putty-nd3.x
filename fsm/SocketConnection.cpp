#include "Reactor.h"
#include "Log.h"
#include "SocketConnection.h"
#include "Protocol.h"
#include "Log.h"
#include "WinProcessor.h"

#include <errno.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define SSIZE_MAX 32767
#define open _open
#define read _read
#ifndef fstat
#define fstat _fstati64
#endif
#ifndef stat
#define stat _stati64
#endif
#define mode_t int
#endif

using namespace Net;

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
            Reactor* theReactor,
            evutil_socket_t theFd)
    : selfM(this)
    , heartbeatTimerEvtM(NULL)
    , clientTimerEvtM(NULL)
    , heartbeatTimeoutCounterM(0)
    , protocolM(theProtocol)
    , reactorM(theReactor)
    , fdM(theFd)
    , inputQueueM(theProtocol->getRBufferSizePower())
    , outputQueueM(theProtocol->getWBufferSizePower())
    , statusM(ActiveE)
    , stopReadingM(false)
    , clientM(NULL)
    , isConnectedNotified(true)
    , uppperDataM(NULL)
{
    readEvtM = reactorM->newEvent(fdM, EV_READ, on_read, this);
    writeEvtM = reactorM->newEvent(fdM, EV_WRITE, on_write, this);
    addReadEvent();
    protocolM->asynHandleConnected(fdM, selfM);
	g_ui_processor->process(NEW_PROCESSOR_JOB(&SocketConnection::startHeartbeatTimer, this));
}

//-----------------------------------------------------------------------------

//SocketConnection::SocketConnection(
//            IProtocol* theProtocol,
//            Reactor* theReactor,
//            evutil_socket_t theFd,
//            Client::TcpClient* theClient)
//    : selfM(this)
//    , heartbeatTimerEvtM(NULL)
//    , clientTimerEvtM(NULL)
//    , heartbeatTimeoutCounterM(0)
//    , protocolM(theProtocol)
//    , reactorM(theReactor)
//    , g_ui_processor(theProcessor)
//    , fdM(theFd)
//    , inputQueueM(theProtocol->getRBufferSizePower())
//    , outputQueueM(theProtocol->getWBufferSizePower())
//    , statusM(ActiveE)
//    , stopReadingM(false)
//    , watcherM(NULL)
//    , clientM(theClient)
//    , isConnectedNotified(false)
//    , uppperDataM(NULL)
//{
//    readEvtM = reactorM->newEvent(fdM, EV_READ, on_read, this);
//    writeEvtM = reactorM->newEvent(fdM, EV_WRITE, on_write, this);
//    addWriteEvent();
//    addReadEvent();
//}
//-----------------------------------------------------------------------------

SocketConnection::~SocketConnection()
{
    evutil_closesocket(fdM);
    if (uppperDataM != NULL)
    {
        LOG_WARN("uppperDataM is not NULL and may leek, please free it and set to NULL in upper handling while connection is closed.");
    }
    LOG_DEBUG("close fd:" << fdM);
}

//-----------------------------------------------------------------------------

void SocketConnection::rmClient()
{
    AutoLock lock(clientMutexM);
    clientM = NULL;
}

//-----------------------------------------------------------------------------

void SocketConnection::addReadEvent()
{
    if (CloseE == statusM)
        return;
    if (-1 == event_add(readEvtM, NULL))
    {
		g_ui_processor->process(NEW_PROCESSOR_JOB(&SocketConnection::addReadEvent, this));
    }
}

//-----------------------------------------------------------------------------

void SocketConnection::addWriteEvent()
{
    if (CloseE == statusM)
        return;
    if (-1 == event_add(writeEvtM, NULL))
    {
		g_ui_processor->process(NEW_PROCESSOR_JOB(&SocketConnection::addWriteEvent, this));
    }
}

//-----------------------------------------------------------------------------

int SocketConnection::asynRead(int theFd, short theEvt)
{
	return g_ui_processor->process(NEW_PROCESSOR_JOB(&SocketConnection::onRead, this, theFd, theEvt));
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
            AutoLock lock(stopReadingMutexM);
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

    while(len > 0 && (BufferOkE == inputQueueM.getStatus()
                    || BufferLowE == inputQueueM.getStatus() ))
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

    if (BufferHighE == inputQueueM.getStatus()
            || BufferNotEnoughE == inputQueueM.getStatus())
    {
        //TRACE("Flow Control:Socket " << fdM << " stop reading.", fdM);
        AutoLock lock(stopReadingMutexM);
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
        BufferStatus postBufferStatus = inputQueueM.getStatus();
        if (postBufferStatus == BufferLowE)
        {
            {
                AutoLock lock(stopReadingMutexM);
                stopReadingM = false;
            }
            asynRead(fdM, 0);
            //g_ui_processor->process(fdM, &SocketConnection::addReadEvent, selfM);
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
        BufferStatus postBufferStatus = inputQueueM.getStatus();
        if (postBufferStatus == BufferLowE)
        {
            {
                AutoLock lock(stopReadingMutexM);
                stopReadingM = false;
            }
            asynRead(fdM, 0);
            //g_ui_processor->process(fdM, &SocketConnection::addReadEvent, selfM);
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
        AutoLock lock(outputQueueMutexM);
        len = outputQueueM.putn(theBuffer, theLen);
    }
    if (0 == len)
    {
        LOG_WARN("outage of the connection's write queue!");
    }
	g_ui_processor->process(NEW_PROCESSOR_JOB(&SocketConnection::addWriteEvent, selfM));
    return len;
}

//-----------------------------------------------------------------------------

int SocketConnection::asynWrite(int theFd, short theEvt)
{
    if (CloseE == statusM)
        return -1;
	return g_ui_processor->process(NEW_PROCESSOR_JOB(&SocketConnection::onWrite, this, theFd, theEvt));
}

//-----------------------------------------------------------------------------
//
//void SocketConnection::setLowWaterMarkWatcher(Watcher* theWatcher)
//{
//	//if it is already writable
//    BufferStatus bufferStatus = outputQueueM.getStatus();
//	if (bufferStatus == BufferLowE)
//	{
//		(*theWatcher)(fdM, selfM);
//		return;
//	}
//
//	//or set theWatcher and add the write event
//	{
//		AutoLock lock(watcherMutexM);
//		if (watcherM)
//		{
//			delete watcherM;
//		}
//		watcherM = theWatcher;
//	}
//    if (CloseE != statusM)
//    {
//        addWriteEvent();
//    }
//}
//
//-----------------------------------------------------------------------------

void SocketConnection::onWrite(int theFd, short theEvt)
{
    if (!isConnectedNotified && clientM)
    {
        //AutoLock lock(clientMutexM);
        //if (clientM)
        //{
        //    clientM->onConnected(theFd, selfM);
        //    isConnectedNotified = true;
        //    startHeartbeatTimer();
        //}
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

    BufferStatus bufferStatus = outputQueueM.getStatus();
    //if (watcherM && (bufferStatus == BufferLowE))
    //{
    //    AutoLock lock(watcherMutexM);
    //    if (watcherM)
    //    {
    //        (*watcherM)(fdM, selfM);
    //        delete watcherM;
    //        watcherM = NULL;
    //    }
    //}

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

    heartbeatTimerEvtM = g_ui_processor->addLocalTimer(tv, &SocketConnection::onHeartbeat, this);
}

//-----------------------------------------------------------------------------

void SocketConnection::onHeartbeat(void *theArg)
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
	g_ui_processor->process(NEW_PROCESSOR_JOB(&SocketConnection::_addClientTimer, this, theSec));
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
        g_ui_processor->cancelLocalTimer(clientTimerEvtM);
    }

    struct timeval tv;
    tv.tv_sec = theSec; 
    tv.tv_usec = 0;

    clientTimerEvtM = g_ui_processor->addLocalTimer(tv, &SocketConnection::onClientTimeout, this);

}

//-----------------------------------------------------------------------------

void SocketConnection::onClientTimeout(void *theArg)
{
    SocketConnection* connection = (SocketConnection*) theArg;
    AutoLock lock(connection->clientMutexM);
    connection->clientTimerEvtM = NULL;
    //if (connection->clientM)
    //{
    //    connection->clientM->onClientTimeout();
    //}

}

//-----------------------------------------------------------------------------

void SocketConnection::close()
{
    if (CloseE == statusM)
        return;
	g_ui_processor->process(NEW_PROCESSOR_JOB( &SocketConnection::_close, this));
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
        g_ui_processor->cancelLocalTimer(heartbeatTimerEvtM);
    }
    if (clientTimerEvtM)
    {
        g_ui_processor->cancelLocalTimer(clientTimerEvtM);
    }
	protocolM->asynHandleClose(fdM, selfM);
    if (clientM)
    {
        AutoLock lock(clientMutexM);
        //if (clientM)
        //{
        //    clientM->onError();
        //    clientM = NULL;
        //}
    }
    reactorM->delEvent(readEvtM);
    reactorM->delEvent(writeEvtM);
    g_ui_processor->process(NEW_PROCESSOR_JOB( &SocketConnection::_release, this));
}

//-----------------------------------------------------------------------------


