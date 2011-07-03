
#ifndef __view_native_listbox_win_h__
#define __view_native_listbox_win_h__

#pragma once

#include "native_listbox_wrapper.h"
#include "view/controls/native_control_win.h"

namespace view
{

    // A View that hosts a native Windows listbox.
    class NativeListboxWin : public NativeControlWin,
        public NativeListboxWrapper
    {
    public:
        NativeListboxWin(Listbox* listbox);
        virtual ~NativeListboxWin();

        // NativeListboxWrapper implementation:
        virtual void UpdateFromModel();
        virtual void UpdateEnabled();
        virtual int GetRowCount() const;
        virtual int SelectedRow() const;
        virtual void SelectRow(int row);
        virtual gfx::Size GetPreferredSize();
        virtual View* GetView();
        virtual void SetFocus();
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

        // The listbox we are bound to.
        Listbox* listbox_;

        // The min width, in pixels, for the text content.
        int content_width_;

        DISALLOW_COPY_AND_ASSIGN(NativeListboxWin);
    };

} //namespace view

#endif //__view_native_listbox_win_h__