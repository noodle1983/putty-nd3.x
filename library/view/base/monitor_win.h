
#ifndef __view_monitor_win_h__
#define __view_monitor_win_h__

#pragma once

namespace gfx
{
    class Rect;
}

namespace view
{

    // Returns the bounds for the monitor that contains the largest area of
    // intersection with the specified rectangle.
    gfx::Rect GetMonitorBoundsForRect(const gfx::Rect& rect);

}  // namespace view

#endif //__view_monitor_win_h__