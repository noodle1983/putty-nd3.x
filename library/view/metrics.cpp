
#include "metrics.h"

#include <windows.h>

namespace view
{

    const int kDefaultMenuShowDelay = 400;

    const int kWheelScrollLines = 3;

    int GetDoubleClickInterval()
    {
        return ::GetDoubleClickTime();
    }

    int GetMenuShowDelay()
    {
        static DWORD delay = 0;
        if(!delay && !SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &delay, 0))
        {
            delay = kDefaultMenuShowDelay;
        }
        return delay;
    }

    int GetWheelScrollLines()
    {
        static int wheel_scroll_lines = 0;
        if(!wheel_scroll_lines && !SystemParametersInfo(
            SPI_GETWHEELSCROLLLINES, 0, &wheel_scroll_lines, 0))
        {
            wheel_scroll_lines = kWheelScrollLines;
        }
        return wheel_scroll_lines;
    }

} //namespace view