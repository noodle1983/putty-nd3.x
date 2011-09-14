
#include "event_utils.h"

#include "view/event/event.h"

namespace event_utils
{

    WindowOpenDisposition DispositionFromClick(bool middle_button,
        bool alt_key,
        bool ctrl_key,
        bool meta_key,
        bool shift_key)
    {
        if(middle_button || ctrl_key)
        {
            return shift_key ? NEW_FOREGROUND_TAB : NEW_BACKGROUND_TAB;
        }
        if(shift_key)
        {
            return NEW_WINDOW;
        }
        if(alt_key)
        {
            return SAVE_TO_DISK;
        }
        return CURRENT_TAB;
    }

    WindowOpenDisposition DispositionFromEventFlags(int event_flags)
    {
        return DispositionFromClick(
            (event_flags & ui::EF_MIDDLE_BUTTON_DOWN) != 0,
            (event_flags & ui::EF_ALT_DOWN) != 0,
            (event_flags & ui::EF_CONTROL_DOWN) != 0,
            (event_flags & ui::EF_COMMAND_DOWN) != 0,
            (event_flags & ui::EF_SHIFT_DOWN) != 0);
    }

    bool IsPossibleDispositionEvent(const view::MouseEvent& event)
    {
        return event.IsLeftMouseButton() || event.IsMiddleMouseButton();
    }

}