
#ifndef __ui_base_mouse_wheel_util_h__
#define __ui_base_mouse_wheel_util_h__

#pragma once

#include <windows.h>

namespace ui
{

    class ViewProp;

    // Marks the passed |hwnd| as supporting mouse-wheel message rerouting.
    // We reroute the mouse wheel messages to such HWND when they are under the
    // mouse pointer (but are not the active window). Callers own the returned
    // object.
    ViewProp* SetWindowSupportsRerouteMouseWheel(HWND hwnd);

    // Forwards mouse wheel messages to the window under it.
    // Windows sends mouse wheel messages to the currently active window.
    // This causes a window to scroll even if it is not currently under the mouse
    // wheel. The following code gives mouse wheel messages to the window under the
    // mouse wheel in order to scroll that window. This is arguably a better user
    // experience.  The returns value says whether the mouse wheel message was
    // successfully redirected.
    bool RerouteMouseWheel(HWND window, WPARAM w_param, LPARAM l_param);

} //namespace ui

#endif //__ui_base_mouse_wheel_util_h__