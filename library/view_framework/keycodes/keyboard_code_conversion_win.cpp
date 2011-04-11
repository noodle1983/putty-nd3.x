
#include "keyboard_code_conversion_win.h"

namespace view
{

    WORD WindowsKeyCodeForKeyboardCode(view::KeyboardCode keycode)
    {
        return static_cast<WORD>(keycode);
    }

    view::KeyboardCode KeyboardCodeForWindowsKeyCode(WORD keycode)
    {
        return static_cast<view::KeyboardCode>(keycode);
    }

} //namespace view