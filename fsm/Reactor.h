#ifndef REACTOR_H
#define REACTOR_H

#include "../libevent/include/event.h"
#include "Singleton.hpp"

namespace Net
{
    class Reactor
    {
    public:
        Reactor();
        ~Reactor();

        void start();
        void dispatchLoop();
        void stop();

        inline struct event *newEvent(
                evutil_socket_t theSocket,
                short theEvt,
                event_callback_fn theFn,
                void* theArg);
        inline void delEvent(struct event*& theEvent);

        inline struct event *newTimer(
                event_callback_fn theFn,
                void* theArg);

        inline struct event *newPersistTimer(
                event_callback_fn theFn,
                void* theArg);
    private:
        struct event_base* evtBaseM;
        struct event* heartbeatEventM;//to avoid the event loop exit

        static Reactor* reactorM;
    };

//-----------------------------------------------------------------------------
struct event* Reactor::newEvent(
                evutil_socket_t theSocket,
                short theEvt,
                event_callback_fn theFn,
                void* theArg)
{
    return event_new(evtBaseM, theSocket, theEvt, theFn, theArg);
}

//-----------------------------------------------------------------------------

void Reactor::delEvent(struct event*& theEvent)
{
    event_del(theEvent);
    event_free(theEvent);
    theEvent = NULL;
}

//-----------------------------------------------------------------------------
struct event* Reactor::newTimer(
                event_callback_fn theFn,
                void* theArg)
{
    return evtimer_new(evtBaseM, theFn, theArg);
}
//-----------------------------------------------------------------------------
struct event* Reactor::newPersistTimer(
                event_callback_fn theFn,
                void* theArg)
{
    return event_new(evtBaseM,
                    -1, 
                    EV_PERSIST, 
                    theFn, 
                    theArg); 
}
}

#define g_ui_reactor (DesignPattern::Singleton<Net::Reactor, 0>::instance())


#endif /* REACTOR_H */
