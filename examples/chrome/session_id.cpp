
#include "session_id.h"

static SessionID::id_type next_id = 1;

SessionID::SessionID()
{
    id_ = next_id++;
}