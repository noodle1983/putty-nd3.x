
#ifndef __ui_base_text_input_type_h__
#define __ui_base_text_input_type_h__

#pragma once

namespace ui
{

    // Intentionally keep sync with WebKit::WebTextInputType defined in:
    // third_party/WebKit/Source/WebKit/chromium/public/WebTextInputType.h
    enum TextInputType
    {
        // Input caret is not in an editable node, no input method shall be used.
        TEXT_INPUT_TYPE_NONE,

        // Input caret is in a normal editable node, any input method can be used.
        TEXT_INPUT_TYPE_TEXT,

        // Input caret is in a password box, an input method may be used only if
        // it's suitable for password input.
        TEXT_INPUT_TYPE_PASSWORD,

        TEXT_INPUT_TYPE_SEARCH,
        TEXT_INPUT_TYPE_EMAIL,
        TEXT_INPUT_TYPE_NUMBER,
        TEXT_INPUT_TYPE_TELEPHONE,
        TEXT_INPUT_TYPE_URL,
    };

} //namespace ui

#endif  //__ui_base_text_input_type_h__