#include "Action.h"
#include "Session.h"
#include "State.h"

#include <string>

using namespace Fsm;
//-----------------------------------------------------------------------------

void Fsm::changeState(
        Fsm::Session* theSession,
        const int theNextStateId)
{
    //exec EXIT FUNC
    {
        State& curState = theSession->getCurState();

        ActionList& actionList = curState.getActionList(EXIT_EVT);
        ActionList::iterator it = actionList.begin();
        if (it != actionList.end())
        {
            LOG_DEBUG(theSession->getSessionName() 
                    << "[" << theSession->getSessionId() << "] handleEvent("
                    << theSession->getEventName(EXIT_EVT) << ")");
            for (; it != actionList.end(); it++)
            {
                Action action = *it;
				action(theSession);
            }
        }
    }

    //exec ENTRY_EVT
    {
        State& nextState = theSession->toNextState(theNextStateId);

        ActionList& actionList = nextState.getActionList(ENTRY_EVT);
        ActionList::iterator it = actionList.begin();
        if (it != actionList.end())
        {
            LOG_DEBUG(theSession->getSessionName() 
                    << "[" << theSession->getSessionId() << "] handleEvent("
                    << theSession->getEventName(ENTRY_EVT) << ")");
            for (; it != actionList.end(); it++)
            {
                Action action = *it;
				action(theSession);
            }
        }
    }
    return ;
}

//-----------------------------------------------------------------------------

void Fsm::generateEvent(
        Fsm::Session* theSession,
        const int theEventId)
{
    theSession->handleEvent(theEventId);
}

//-----------------------------------------------------------------------------

void Fsm::ignoreEvent(
        Fsm::Session* theSession)
{
    LOG_DEBUG(theSession->getSessionName() 
            << " ignore event.");
}

//-----------------------------------------------------------------------------
void Fsm::newTimer(
        Fsm::Session* theSession,
        const long theMsec)
{
    theSession->newTimer(theMsec);
}

//-----------------------------------------------------------------------------

void Fsm::newFuncTimer(
		Fsm::Session* theSession,
		TimerGettor theTimerGettor)
{
	long msec = theTimerGettor();
	return Fsm::newTimer(theSession, msec);
}

//-----------------------------------------------------------------------------
void Fsm::cancelTimer(
        Fsm::Session* theSession)
{
    theSession->cancelTimer();
}

//-----------------------------------------------------------------------------

void Fsm::deleteSession(
        Fsm::Session* theSession)
{
    delete theSession;
}

//-----------------------------------------------------------------------------

