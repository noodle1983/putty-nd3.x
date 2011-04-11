
#include "event_utils_win.h"

#include "event.h"

namespace view
{

    int GetRepeatCount(const KeyEvent& event)
    {
        return LOWORD(event.native_event().lParam);
    }

    bool IsExtendedKey(const KeyEvent& event)
    {
        return (HIWORD(event.native_event().lParam) & KF_EXTENDED) == KF_EXTENDED;
    }

    int GetWindowsFlags(const Event& event)
    {
        // TODO(beng): need support for x1/x2.
        int result = 0;
        result |= (event.flags() & EF_SHIFT_DOWN) ? MK_SHIFT : 0;
        result |= (event.flags() & EF_CONTROL_DOWN) ? MK_CONTROL : 0;
        result |= (event.flags() & EF_LEFT_BUTTON_DOWN) ? MK_LBUTTON : 0;
        result |= (event.flags() & EF_MIDDLE_BUTTON_DOWN) ? MK_MBUTTON : 0;
        result |= (event.flags() & EF_RIGHT_BUTTON_DOWN) ? MK_RBUTTON : 0;
        return result;
    }

} //namespace view