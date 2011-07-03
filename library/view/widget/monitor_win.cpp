
#include "monitor_win.h"

#include <windows.h>

#include "base/logging.h"

#include "ui_gfx/rect.h"

namespace view
{

    gfx::Rect GetMonitorBoundsForRect(const gfx::Rect& rect)
    {
        RECT p_rect = rect.ToRECT();
        HMONITOR monitor = MonitorFromRect(&p_rect, MONITOR_DEFAULTTONEAREST);
        if(monitor)
        {
            MONITORINFO mi = { 0 };
            mi.cbSize = sizeof(mi);
            GetMonitorInfo(monitor, &mi);
            return gfx::Rect(mi.rcWork);
        }
        NOTREACHED();
        return gfx::Rect();
    }

} //namespace view