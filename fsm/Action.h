#ifndef ACTION_H
#define ACTION_H

#include "FsmInterface.h"

#define CHANGE_STATE(theStateId)  FSM_BIND(Fsm::changeState,   _1, (theStateId))
#define GEN_EVT(theEventId)       FSM_BIND(Fsm::generateEvent, _1, (theEventId))
#define NEW_TIMER(theMsec)        FSM_BIND(Fsm::newTimer,      _1, (theMsec))
#define NEW_FUNC_TIMER(theGettor) FSM_BIND(Fsm::newFuncTimer,  _1, Fsm::TimerGettor(theGettor))
#define CANCEL_TIMER()            (&Fsm::cancelTimer)
#define DELETE_SESSION()          (&Fsm::deleteSession)
#define IGNORE_EVT()              (&Fsm::ignoreEvent)

namespace Fsm
{
    class Session;
    typedef FSM_FUNCTION<void (Fsm::Session *)> Action;
    typedef FSM_FUNCTION<long ()> TimerGettor;

    void changeState(
            Fsm::Session* theSession,
            const int theNextStateId);

    void generateEvent(
            Fsm::Session* theSession,
            const int theEventId);

    void ignoreEvent(
            Fsm::Session* theSession);

    void newTimer(
            Fsm::Session* theSession,
            const long theMsec);

    void newFuncTimer(
            Fsm::Session* theSession,
            TimerGettor theTimerGettor);

    void cancelTimer(
            Fsm::Session* theSession);

    void deleteSession(
            Fsm::Session* theSession);
}

#endif /* ACTION_H */
