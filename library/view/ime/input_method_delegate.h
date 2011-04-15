
#ifndef __view_input_method_delegate_h__
#define __view_input_method_delegate_h__

#pragma once

namespace view
{

    class KeyEvent;

    // An interface implemented by the object that handles events sent back from an
    // InputMethod implementation.
    class InputMethodDelegate
    {
    public:
        virtual ~InputMethodDelegate() {}

        // Dispatch a key event already processed by the input method.
        virtual void DispatchKeyEventPostIME(const KeyEvent& key) = 0;

    };

} //namespace view

#endif //__view_input_method_delegate_h__