
#include "scrollbar_size.h"

#include <windows.h>

namespace gfx
{

    int scrollbar_size()
    {
        return GetSystemMetrics(SM_CXVSCROLL);
    }

} //namespace gfx