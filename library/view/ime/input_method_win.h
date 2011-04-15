
#ifndef __view_input_method_win_h__
#define __view_input_method_win_h__

#pragma once

#include "../widget/widget.h"
#include "ime_input.h"
#include "input_method_base.h"

namespace view
{

    // An InputMethod implementation based on Windows IMM32 API.
    class InputMethodWin : public InputMethodBase
    {
    public:
        explicit InputMethodWin(InputMethodDelegate* delegate);
        virtual ~InputMethodWin();

        // Overridden from InputMethod:
        virtual void Init(Widget* widget);
        virtual void OnFocus();
        virtual void OnBlur();
        virtual void DispatchKeyEvent(const KeyEvent& key);
        virtual void OnTextInputTypeChanged(View* view);
        virtual void OnCaretBoundsChanged(View* view);
        virtual void CancelComposition(View* view);
        virtual std::string GetInputLocale();
        virtual base::TextDirection GetInputTextDirection();
        virtual bool IsActive();

        // Message handlers. The native widget is responsible for forwarding following
        // messages to the input method.
        void OnInputLangChange(DWORD character_set, HKL input_language_id);
        LRESULT OnImeSetContext(
            UINT message, WPARAM wparam, LPARAM lparam, BOOL* handled);
        LRESULT OnImeStartComposition(
            UINT message, WPARAM wparam, LPARAM lparam, BOOL* handled);
        LRESULT OnImeComposition(
            UINT message, WPARAM wparam, LPARAM lparam, BOOL* handled);
        LRESULT OnImeEndComposition(
            UINT message, WPARAM wparam, LPARAM lparam, BOOL* handled);
        // For both WM_CHAR and WM_SYSCHAR
        LRESULT OnChar(
            UINT message, WPARAM wparam, LPARAM lparam, BOOL* handled);
        // For both WM_DEADCHAR and WM_SYSDEADCHAR
        LRESULT OnDeadChar(
            UINT message, WPARAM wparam, LPARAM lparam, BOOL* handled);

    private:
        // Overridden from InputMethodBase.
        virtual void FocusedViewWillChange();
        virtual void FocusedViewDidChange();

        // A helper function to return the Widget's native window.
        HWND hwnd() const { return widget()->GetNativeView(); }

        // Asks the client to confirm current composition text.
        void ConfirmCompositionText();

        // Enables or disables the IME according to the current text input type.
        void UpdateIMEState();

        // Indicates if the current input locale has an IME.
        bool active_;

        // Name of the current input locale.
        std::string locale_;

        // The current input text direction.
        base::TextDirection direction_;

        // The new text direction and layout alignment requested by the user by
        // pressing ctrl-shift. It'll be sent to the text input client when the key
        // is released.
        base::TextDirection pending_requested_direction_;

        // Windows IMM32 wrapper.
        // (See "ui/base/win/ime_input.h" for its details.)
        ImeInput ime_input_;

        DISALLOW_COPY_AND_ASSIGN(InputMethodWin);
    };

} //namespace view

#endif //__view_input_method_win_h__