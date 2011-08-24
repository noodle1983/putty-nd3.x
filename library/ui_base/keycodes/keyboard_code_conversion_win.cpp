
#include "keyboard_code_conversion_win.h"

namespace ui
{

    WORD WindowsKeyCodeForKeyboardCode(KeyboardCode keycode)
    {
        return static_cast<WORD>(keycode);
    }

    KeyboardCode KeyboardCodeForWindowsKeyCode(WORD keycode)
    {
        return static_cast<KeyboardCode>(keycode);
    }

} //namespace ui