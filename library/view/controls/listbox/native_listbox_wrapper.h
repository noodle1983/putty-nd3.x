
#ifndef __view_native_listbox_wrapper_h__
#define __view_native_listbox_wrapper_h__

#pragma once

#include "listbox.h"

namespace view
{

    // An interface implemented by an object that provides a platform-native
    // listbox.
    class NativeListboxWrapper
    {
    public:
        // Returns the number of rows in the table.
        virtual int GetRowCount() const = 0;

        // Returns the 0-based index of the currently selected row.
        virtual int SelectedRow() const = 0;

        // Selects the specified row, making sure it's visible.
        virtual void SelectRow(int row) = 0;

        // Retrieves the views::View that hosts the native control.
        virtual View* GetView() = 0;

        // Updates the enabled state of the combobox from the associated view.
        virtual void UpdateEnabled() = 0;

        // Returns the preferred size of the combobox.
        virtual gfx::Size GetPreferredSize() = 0;

        // Sets the focus to the listbox.
        virtual void SetFocus() = 0;

        // Creates an appropriate NativeListboxWrapper for the platform.
        static NativeListboxWrapper* CreateNativeWrapper(
            Listbox* listbox,
            const std::vector<string16>& strings,
            Listbox::Listener* listener);

    protected:
        virtual ~NativeListboxWrapper() {}
    };

} //namespace view

#endif //__view_native_listbox_wrapper_h__