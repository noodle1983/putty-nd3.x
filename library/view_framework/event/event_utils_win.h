
#ifndef __view_framework_event_utils_win_h__
#define __view_framework_event_utils_win_h__

#pragma once

namespace view
{

    class Event;
    class KeyEvent;

    // Returns the repeat count of the specified KeyEvent. Valid only for
    // KeyEvents constructed from a MSG.
    int GetRepeatCount(const KeyEvent& event);

    // Returns true if the affected key is a Windows extended key. See documentation
    // for WM_KEYDOWN for explanation.
    // Valid only for KeyEvents constructed from a MSG.
    bool IsExtendedKey(const KeyEvent& event);

    // Return a mask of windows key/button state flags for the event object.
    int GetWindowsFlags(const Event& event);

} //namespace view

#endif //__view_framework_event_utils_win_h__