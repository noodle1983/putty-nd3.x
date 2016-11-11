#ifndef FSM_H
#define FSM_H

#include "State.h"
#include "Log.h"
#include <map>

namespace Fsm
{
    class FiniteStateMachine
    {
    public:
        enum {STATE_NONE = -1};
        FiniteStateMachine()
            : initStateIdM(STATE_NONE)
            , endStateIdM(STATE_NONE)
        {
        }

        ~FiniteStateMachine(){}

        FiniteStateMachine& operator+=(const State& theState);
        FiniteStateMachine& operator+=(const Event& theEvent);
        const std::string& getEventName(const int theEventName);

        inline State& getState(const int theStateId)
        {
            std::map<int, State>::iterator it = statesM.find(theStateId);
            if (it == statesM.end())
            {
                LOG_FATAL("failed to find state with id" << theStateId);
                exit(-1);
            }
            return it->second;
        }

        inline int getFirstStateId()
        {
            return initStateIdM;
        }

        inline int getLastStateId()
        {
            return endStateIdM;
        }

    private:
        std::map<int, State> statesM;
        std::map<int, std::string> eventNamesM;
        int initStateIdM;
        int endStateIdM;
    };

}
#endif /* FSM_H */

