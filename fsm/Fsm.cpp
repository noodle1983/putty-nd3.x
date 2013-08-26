#include "Fsm.h"

using namespace Fsm;
//-----------------------------------------------------------------------------

FiniteStateMachine& FiniteStateMachine::operator+=(const State& theState)
{
    int stateId = theState.getId();
    if (STATE_NONE == initStateIdM)
    {
        initStateIdM = stateId;
    }
    if (statesM.find(stateId) != statesM.end())
    {
        LOG_FATAL("exit because fsm state " << theState.getName()
                << " is redefined!");
        exit(-1);
    }
    endStateIdM = stateId;

    statesM[theState.getId()] = theState;
    return *this;
}

//-----------------------------------------------------------------------------

FiniteStateMachine& FiniteStateMachine::operator+=(const Event& theEvent)
{
    eventNamesM[theEvent.getId()] = theEvent.getName();
    State& curState = statesM[endStateIdM];
    curState.addEvent(theEvent);
    return *this;
}

//-----------------------------------------------------------------------------

const std::string& FiniteStateMachine::getEventName(const int theEventName)
{
    std::map<int, std::string>::iterator it = eventNamesM.find(theEventName);
    if (it != eventNamesM.end())
    {
        return it->second;
    }
    else
    {
        static const std::string emptyString; 
        return emptyString;
    }
}

//-----------------------------------------------------------------------------

