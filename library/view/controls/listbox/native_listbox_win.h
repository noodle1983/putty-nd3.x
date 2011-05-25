
#ifndef __view_native_listbox_win_h__
#define __view_native_listbox_win_h__

#pragma once

#include "../native_control_win.h"
#include "native_listbox_wrapper.h"

namespace view
{

    // A View that hosts a native Windows listbox.
    class NativeListboxWin : public NativeControlWin, public NativeListboxWrapper
    {
    public:
        NativeListboxWin(Listbox* listbox,
            const std::vector<string16>& strings,
            Listbox::Listener* listener);
        virtual ~NativeListboxWin();

        // NativeListboxWrapper implementation:
        virtual int GetRowCount() const;
        virtual int SelectedRow() const;
        virtual void SelectRow(int row);
        virtual View* GetView();
        virtual void UpdateEnabled();
        virtual gfx::Size GetPreferredSize();
        virtual void SetFocus();

        // Overridden from NativeControlWin:
        virtual bool ProcessMessage(UINT message,
            WPARAM w_param,
            LPARAM l_param,
            LRESULT* result);

    protected:
        virtual void CreateNativeControl();

    private:
        // The listbox we are bound to.
        Listbox* listbox_;

        // The strings shown in the listbox.
        std::vector<string16> strings_;

        // Listens to selection changes.
        Listbox::Listener* listener_;

        DISALLOW_COPY_AND_ASSIGN(NativeListboxWin);
    };

} //namespace view

#endif //__view_native_listbox_win_h__