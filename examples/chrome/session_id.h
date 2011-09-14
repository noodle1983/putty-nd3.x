
#ifndef __session_id_h__
#define __session_id_h__

#pragma once

#include "base/basic_types.h"

// Uniquely identifies a tab or window for the duration of a session.
class SessionID
{
public:
    typedef int32 id_type;

    SessionID();
    ~SessionID() {}

    // Returns the underlying id.
    void set_id(id_type id) { id_ = id; }
    id_type id() const { return id_; }

private:
    id_type id_;
};

#endif //__session_id_h__