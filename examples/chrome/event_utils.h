
#ifndef __event_utils_h__
#define __event_utils_h__

#pragma once

#include "window_open_disposition.h"

namespace view
{
    class MouseEvent;
}

namespace event_utils
{

    // Translates event flags from a click on a link into the user's desired
    // window disposition.  For example, a middle click would mean to open
    // a background tab.
    WindowOpenDisposition DispositionFromClick(bool middle_button,
        bool alt_key,
        bool ctrl_key,
        bool meta_key,
        bool shift_key);

    // Translates event flags into what kind of disposition they represents.
    // For example, a middle click would mean to open a background tab.
    // event_flags are the flags as understood by views::MouseEvent.
    WindowOpenDisposition DispositionFromEventFlags(int event_flags);

    // Returns true if the specified mouse event may have a
    // WindowOptionDisposition.
    bool IsPossibleDispositionEvent(const view::MouseEvent& event);

}

#endif //__event_utils_h__