
#ifndef __view_text_input_type_h__
#define __view_text_input_type_h__

#pragma once

namespace view
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

        // TODO(suzhe): Add more text input types when necessary, eg. Number, Date,
        // Email, URL, etc.
    };

} //namespace view

#endif //__view_text_input_type_h__