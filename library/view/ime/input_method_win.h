
#ifndef __view_input_method_win_h__
#define __view_input_method_win_h__

#pragma once

#include "view/focus/focus_manager.h"

namespace view
{

    // 辅助类, 为widget提供输入法管理.
    // 记录当前具有焦点的视图, 对其派发输入消息.
    class InputMethodWin : public FocusChangeListener
    {
    public:
        InputMethodWin();
        virtual ~InputMethodWin();

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

        // Overridden from FocusChangeListener. Derived classes shouldn't override
        // this method. Override FocusedViewWillChange() and FocusedViewDidChange()
        // instead.
        virtual void FocusWillChange(View* focused_before, View* focused);

        // Message handlers. The native widget is responsible for forwarding following
        // messages to the input method.
        void OnInputLangChange(DWORD character_set, HKL input_language_id);
        LRESULT OnImeSetContext(UINT message, WPARAM wparam,
            LPARAM lparam, BOOL* handled);
        LRESULT OnImeStartComposition(UINT message, WPARAM wparam,
            LPARAM lparam, BOOL* handled);
        LRESULT OnImeComposition(UINT message, WPARAM wparam,
            LPARAM lparam, BOOL* handled);
        LRESULT OnImeEndComposition(UINT message, WPARAM wparam,
            LPARAM lparam, BOOL* handled);
        LRESULT OnImeNotify(UINT message, WPARAM wparam,
            LPARAM lparam, BOOL* handled);
        // For both WM_CHAR and WM_SYSCHAR
        LRESULT OnChar(UINT message, WPARAM wparam,
            LPARAM lparam, BOOL* handled);
        // For both WM_DEADCHAR and WM_SYSDEADCHAR
        LRESULT OnDeadChar(UINT message, WPARAM wparam,
            LPARAM lparam, BOOL* handled);

    protected:
        // Getters and setters of properties.
        Widget* widget() const { return widget_; }
        View* focused_view() const { return focused_view_; }
        bool widget_focused() const { return widget_focused_; }

        // Checks if the given View is focused. Returns true only if the View and
        // the Widget are both focused.
        bool IsViewFocused(View* view) const;

    private:
        Widget* widget_;
        View* focused_view_;

        // Indicates if the top-level widget is focused or not.
        bool widget_focused_;

        DISALLOW_COPY_AND_ASSIGN(InputMethodWin);
    };

} //namespace view

#endif //__view_input_method_win_h__