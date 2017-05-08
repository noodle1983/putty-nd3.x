#include "Reactor.h"
#include "Log.h"

#include "../libevent/include/event.h"
using namespace Net;

#include "WinMutex.h"


//-----------------------------------------------------------------------------

void on_heartbeat(int theFd, short theEvt, void *theArg)
{
    //DEBUG("reactor heartbeat.");
}

//-----------------------------------------------------------------------------

Reactor::Reactor()
{
    evtBaseM = event_base_new();
    heartbeatEventM = NULL;
	start();
}

//-----------------------------------------------------------------------------

Reactor::~Reactor()
{
    event_base_free(evtBaseM);
    if (heartbeatEventM)
    {
        delEvent(heartbeatEventM);
    }
}

//-----------------------------------------------------------------------------

void Reactor::start()
{
    heartbeatEventM = event_new(evtBaseM, -1, EV_PERSIST, on_heartbeat, this);
    struct timeval tv;
    tv.tv_sec = 5;  //5 seconds
    tv.tv_usec = 0;
    event_add(heartbeatEventM, &tv);
}

//-----------------------------------------------------------------------------

void Reactor::dispatchLoop()
{
    LOG_DEBUG("enter event dispatch.");
	/*int ret =*/ event_base_loop(evtBaseM, EVLOOP_NONBLOCK);
    //if (-1 == ret)
    //{
    //    LOG_ERROR("exit event dispatch with error.");
    //}
    //else
    //{
    //    LOG_DEBUG("exit event dispatch.");
    //}

}

//-----------------------------------------------------------------------------

void Reactor::stop()
{
}



