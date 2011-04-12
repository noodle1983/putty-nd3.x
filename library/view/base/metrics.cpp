
#include "metrics.h"

#include <windows.h>

namespace view
{

    const int kDefaultMenuShowDelay = 400;

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

} //namespace view