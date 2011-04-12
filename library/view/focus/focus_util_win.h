
#ifndef __view_focus_util_win_h__
#define __view_focus_util_win_h__

#pragma once

#include <windows.h>

namespace view
{

    class ViewProp;

    // Marks the passed |hwnd| as supporting mouse-wheel message rerouting.
    // We reroute the mouse wheel messages to such HWND when they are under the
    // mouse pointer (but are not the active window)
    ViewProp* SetWindowSupportsRerouteMouseWheel(HWND hwnd);

    // Forwards mouse wheel messages to the window under it.
    // Windows sends mouse wheel messages to the currently active window.
    // This causes a window to scroll even if it is not currently under the mouse
    // wheel. The following code gives mouse wheel messages to the window under the
    // mouse wheel in order to scroll that window. This is arguably a better user
    // experience.  The returns value says whether the mouse wheel message was
    // successfully redirected.
    bool RerouteMouseWheel(HWND window, WPARAM w_param, LPARAM l_param);

} //namespace view

#endif //__view_focus_util_win_h__