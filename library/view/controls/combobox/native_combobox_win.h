
#ifndef __view_native_combobox_win_h__
#define __view_native_combobox_win_h__

#pragma once

#include "native_combobox_wrapper.h"
#include "view/controls/native_control_win.h"

namespace view
{

    class NativeComboboxWin : public NativeControlWin,
        public NativeComboboxWrapper
    {
    public:
        explicit NativeComboboxWin(Combobox* combobox);
        virtual ~NativeComboboxWin();

        // Overridden from NativeComboboxWrapper:
        virtual void UpdateFromModel();
        virtual void UpdateSelectedItem();
        virtual void UpdateEnabled();
        virtual int GetSelectedItem() const;
        virtual bool IsDropdownOpen() const;
        virtual gfx::Size GetPreferredSize();
        virtual View* GetView();
        virtual void SetFocus();
        virtual bool HandleKeyPressed(const KeyEvent& event);
        virtual bool HandleKeyReleased(const KeyEvent& event);
        virtual void HandleFocus();
        virtual void HandleBlur();
        virtual HWND GetTestingHandle() const;

    protected:
        // Overridden from NativeControlWin:
        virtual bool ProcessMessage(UINT message,
            WPARAM w_param,
            LPARAM l_param,
            LRESULT* result);
        virtual void CreateNativeControl();
        virtual void NativeControlCreated(HWND native_control);

    private:
        void UpdateFont();

        // The combobox we are bound to.
        Combobox* combobox_;

        // The min width, in pixels, for the text content.
        int content_width_;

        DISALLOW_COPY_AND_ASSIGN(NativeComboboxWin);
    };

} //namespace view

#endif //__view_native_combobox_win_h__