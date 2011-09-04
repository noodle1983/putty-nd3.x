
#include "monitor_win.h"

#include <shellapi.h>

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

    HWND GetTopmostAutoHideTaskbarForEdge(UINT edge, HMONITOR monitor)
    {
        APPBARDATA taskbar_data =  { sizeof(APPBARDATA), NULL, 0, edge };
        HWND taskbar = reinterpret_cast<HWND>(SHAppBarMessage(ABM_GETAUTOHIDEBAR,
            &taskbar_data));
        return (::IsWindow(taskbar) && (monitor != NULL) &&
            (MonitorFromWindow(taskbar, MONITOR_DEFAULTTONULL) == monitor) &&
            (GetWindowLong(taskbar, GWL_EXSTYLE) & WS_EX_TOPMOST)) ? taskbar : NULL;
    }

} //namespace view