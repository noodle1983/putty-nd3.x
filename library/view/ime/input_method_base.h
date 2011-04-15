
#ifndef __view_input_method_base_h__
#define __view_input_method_base_h__

#pragma once

#include "../focus/focus_manager.h"
#include "input_method.h"
#include "input_method_delegate.h"

namespace gfx
{
    class Rect;
}

namespace view
{

    class View;

    // A helper class providing functionalities shared among InputMethod
    // implementations.
    class InputMethodBase : public InputMethod, public FocusChangeListener
    {
    public:
        InputMethodBase();
        virtual ~InputMethodBase();

        // Overriden from InputMethod.
        virtual void set_delegate(InputMethodDelegate* delegate);

        // If a derived class overrides this method, it should call parent's
        // implementation first.
        virtual void Init(Widget* widget);

        // If a derived class overrides this method, it should call parent's
        // implementation first, to make sure |widget_focused_| flag can be updated
        // correctly.
        virtual void OnFocus();

        // If a derived class overrides this method, it should call parent's
        // implementation first, to make sure |widget_focused_| flag can be updated
        // correctly.
        virtual void OnBlur();

        virtual TextInputClient* GetTextInputClient() const;
        virtual TextInputType GetTextInputType() const;

        // Overridden from FocusChangeListener. Derived classes shouldn't override
        // this method. Override FocusedViewWillChange() and FocusedViewDidChange()
        // instead.
        virtual void FocusWillChange(View* focused_before, View* focused);

    protected:
        // Getters and setters of properties.
        InputMethodDelegate* delegate() const { return delegate_; }
        Widget* widget() const { return widget_; }
        View* focused_view() const { return focused_view_; }
        bool widget_focused() const { return widget_focused_; }

        // Checks if the given View is focused. Returns true only if the View and
        // the Widget are both focused.
        bool IsViewFocused(View* view) const;

        // Checks if the focused text input client's text input type is
        // TEXT_INPUT_TYPE_NONE. Also returns true if there is no focused text
        // input client.
        bool IsTextInputTypeNone() const;

        // Convenience method to call the focused text input client's
        // OnInputMethodChanged() method. It'll only take effect if the current text
        // input type is not TEXT_INPUT_TYPE_NONE.
        void OnInputMethodChanged() const;

        // Convenience method to call delegate_->DispatchKeyEventPostIME().
        void DispatchKeyEventPostIME(const KeyEvent& key) const;

        // Gets the current text input client's caret bounds in Widget's coordinates.
        // Returns false if the current text input client doesn't support text input.
        bool GetCaretBoundsInWidget(gfx::Rect* rect) const;

        // Called just before changing the focused view. Should be overridden by
        // derived classes. The default implementation does nothing.
        virtual void FocusedViewWillChange();

        // Called just after changing the focused view. Should be overridden by
        // derived classes. The default implementation does nothing.
        // Note: It's called just after changing the value of |focused_view_|. As it's
        // called inside FocusChangeListener's FocusWillChange() method, which is
        // called by the FocusManager before actually changing the focus, the derived
        // class should not rely on the actual focus state of the |focused_view_|.
        virtual void FocusedViewDidChange();

    private:
        InputMethodDelegate* delegate_;
        Widget* widget_;
        View* focused_view_;

        // Indicates if the top-level widget is focused or not.
        bool widget_focused_;

        DISALLOW_COPY_AND_ASSIGN(InputMethodBase);
    };

} //namespace view

#endif //__view_input_method_base_h__