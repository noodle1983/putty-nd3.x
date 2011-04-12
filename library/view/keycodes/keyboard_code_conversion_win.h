
#ifndef __view_framework_keyboard_code_conversion_win_h__
#define __view_framework_keyboard_code_conversion_win_h__

#pragma once

#include "keyboard_codes_win.h"

namespace view
{

    // Methods to convert view::KeyboardCode/Windows virtual key type methods.
    WORD WindowsKeyCodeForKeyboardCode(KeyboardCode keycode);
    KeyboardCode KeyboardCodeForWindowsKeyCode(WORD keycode);

} // namespace view

#endif  //__view_framework_keyboard_code_conversion_win_h__