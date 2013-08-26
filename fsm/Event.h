#ifndef EVENT_H
#define EVENT_H

#include "Action.h"

#define FSM_EVENT(ID, ACTION) Fsm::Event((#ID), (ID), (ACTION))

namespace Fsm
{
	enum PreDefineEvent
	{
		ENTRY_EVT     = -1,
		EXIT_EVT      = -2,
		TIMEOUT_EVT   = -3,
		NEXT_EVT      = -4,
		OK_EVT        = -5, 
		FAILED_EVT    = -6

	};
    class Event
    {
    public:

        Event(const std::string& theEvtName, const int theId, Action theAction)
            : nameM(theEvtName)
            , idM(theId)
            , actionM(theAction)
        {}
        ~Event(){};
        
        std::string getName() const
        {
            return nameM;
        }

        int getId() const
        {
            return idM;
        }

        Action getAction() const
        {
            return actionM;
        }
    private:
        std::string nameM;
        int idM;
        Action actionM;

    };
}
#endif /* EVENT_H */

