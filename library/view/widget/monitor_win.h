
#ifndef __view_monitor_win_h__
#define __view_monitor_win_h__

#pragma once

#include <windows.h>

namespace gfx
{
    class Rect;
}

namespace view
{

    // Returns the bounds for the monitor that contains the largest area of
    // intersection with the specified rectangle.
    gfx::Rect GetMonitorBoundsForRect(const gfx::Rect& rect);

    // Returns the always-on-top auto-hiding taskbar for edge |edge| (one of
    // ABE_LEFT, TOP, RIGHT, or BOTTOM) of monitor |monitor|. NULL is returned
    // if nothing is found.
    HWND GetTopmostAutoHideTaskbarForEdge(UINT edge, HMONITOR monitor);

}  // namespace view

#endif //__view_monitor_win_h__