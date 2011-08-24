
#ifndef __ui_base_keyboard_code_conversion_win_h__
#define __ui_base_keyboard_code_conversion_win_h__

#pragma once

#include "keyboard_codes_win.h"

namespace ui
{

    // Methods to convert view::KeyboardCode/Windows virtual key type methods.
    WORD WindowsKeyCodeForKeyboardCode(KeyboardCode keycode);
    KeyboardCode KeyboardCodeForWindowsKeyCode(WORD keycode);

} // namespace ui

#endif  //__ui_base_keyboard_code_conversion_win_h__