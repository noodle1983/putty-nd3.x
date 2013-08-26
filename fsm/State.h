#ifndef STATE_H
#define STATE_H

#include <map>
#include <list>
#include <string>

#include <Event.h>
#include <Action.h>

#define FSM_STATE(ID) Fsm::State((#ID), (ID))

namespace Fsm
{
    typedef std::list<Action> ActionList;
    class State
    {
    public:
        State(){}
        State(const std::string theName, const int theId)
            : idM(theId)
            , stateNameM(theName)
        {}
        ~State(){};

        inline int getId()const {return idM;}
        void addEvent(const Event& theEvent)
        {
            int eventId = theEvent.getId();
            ActionList& actionList = rulesM[eventId];
            actionList.push_back(theEvent.getAction());
        }

        ActionList& getActionList(const int theEventId)
        {
            std::map<int, ActionList>::iterator it = rulesM.find(theEventId);
            if (it == rulesM.end())
            {
                return emptyActionListM;
            }
            else 
            {
                return it->second;
            }
        }

        inline bool isValid()
        {
            return !stateNameM.empty();
        }

        inline const std::string& getName() const
        {
            return stateNameM;
        }

    private:
        int idM;
        std::string stateNameM;
        std::map<int, ActionList> rulesM;
        static ActionList emptyActionListM;
    };
}
#endif /* STATE_H */

