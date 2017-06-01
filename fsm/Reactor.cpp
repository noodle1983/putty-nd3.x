#include "Reactor.h"
#include "Log.h"

#include "../libevent/include/event.h"
using namespace Net;

#include "WinMutex.h"
#include "WinProcessor.h"


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

	extern void loop_in_bg_processor(void* arg);
	g_bg_processor->process(0, NEW_PROCESSOR_JOB(loop_in_bg_processor, (void*)NULL));
}

//-----------------------------------------------------------------------------

void loop_in_bg_processor(void* arg)
{
	g_ui_reactor->dispatchLoop();

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;
	g_bg_processor->addLocalTimer(0, timeout, loop_in_bg_processor, NULL);
}

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



