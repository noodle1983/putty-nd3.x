
#include "input_method_win.h"

#include "base/logging.h"

#include "../view/view.h"
#include "../widget/widget.h"

namespace view
{

    InputMethodWin::InputMethodWin()
        : widget_(NULL),
        focused_view_(NULL),
        widget_focused_(false) {}

    InputMethodWin::~InputMethodWin()
    {
        if(widget_)
        {
            widget_->GetFocusManager()->RemoveFocusChangeListener(this);
            widget_ = NULL;
        }
    }

    void InputMethodWin::Init(Widget* widget)
    {
        DCHECK(widget);
        DCHECK(widget->GetFocusManager());

        if(widget_)
        {
            NOTREACHED() << "The input method is already initialized.";
            return;
        }

        widget_ = widget;
        widget->GetFocusManager()->AddFocusChangeListener(this);
    }

    void InputMethodWin::OnFocus()
    {
        DCHECK(!widget_focused());
        widget_focused_ = true;
    }

    void InputMethodWin::OnBlur()
    {
        DCHECK(widget_focused());
        widget_focused_ = false;
    }

    void InputMethodWin::FocusWillChange(View* focused_before, View* focused)
    {
        DCHECK_EQ(focused_view_, focused_before);
        focused_view_ = focused;
    }

    void InputMethodWin::OnInputLangChange(DWORD character_set,
        HKL input_language_id)
    {
        BOOL handled;
        if(focused_view_)
        {
            focused_view_->OnImeMessages(WM_INPUTLANGCHANGE,
                static_cast<WPARAM>(character_set),
                reinterpret_cast<LPARAM>(input_language_id),
                &handled);
        }
    }

    LRESULT InputMethodWin::OnImeSetContext(UINT message, WPARAM wparam,
        LPARAM lparam, BOOL* handled)
    {
        if(focused_view_)
        {
            return focused_view_->OnImeMessages(message, wparam, lparam, handled);
        }

        return 0;
    }

    LRESULT InputMethodWin::OnImeStartComposition(UINT message, WPARAM wparam,
        LPARAM lparam, BOOL* handled)
    {
        // We have to prevent WTL from calling ::DefWindowProc() because the function
        // calls ::ImmSetCompositionWindow() and ::ImmSetCandidateWindow() to
        // over-write the position of IME windows.
        *handled = TRUE;

        if(focused_view_)
        {
            focused_view_->OnImeMessages(message, wparam, lparam, handled);
        }

        return 0;
    }

    LRESULT InputMethodWin::OnImeComposition(UINT message, WPARAM wparam,
        LPARAM lparam, BOOL* handled)
    {
        // We have to prevent WTL from calling ::DefWindowProc() because we do not
        // want for the IMM (Input Method Manager) to send WM_IME_CHAR messages.
        *handled = TRUE;

        if(focused_view_)
        {
            focused_view_->OnImeMessages(message, wparam, lparam, handled);
        }

        return 0;
    }

    LRESULT InputMethodWin::OnImeEndComposition(UINT message, WPARAM wparam,
        LPARAM lparam, BOOL* handled)
    {
        // Let WTL call ::DefWindowProc() and release its resources.
        *handled = FALSE;

        if(focused_view_)
        {
            focused_view_->OnImeMessages(message, wparam, lparam, handled);
        }

        return 0;
    }

    LRESULT InputMethodWin::OnImeNotify(UINT message, WPARAM wparam,
        LPARAM lparam, BOOL* handled)
    {
        *handled = TRUE;

        if(focused_view_)
        {
            focused_view_->OnImeMessages(message, wparam, lparam, handled);
        }

        return 0;
    }

    LRESULT InputMethodWin::OnChar(UINT message, WPARAM wparam,
        LPARAM lparam, BOOL* handled)
    {
        *handled = TRUE;

        if(focused_view_)
        {
            focused_view_->OnImeMessages(message, wparam, lparam, handled);
        }

        return 0;
    }

    LRESULT InputMethodWin::OnDeadChar(UINT message, WPARAM wparam,
        LPARAM lparam, BOOL* handled)
    {
        *handled = TRUE;

        if(focused_view_)
        {
            focused_view_->OnImeMessages(message, wparam, lparam, handled);
        }

        return 0;
    }

    bool InputMethodWin::IsViewFocused(View* view) const
    {
        return widget_focused_ && view && focused_view_==view;
    }

} //namespace view