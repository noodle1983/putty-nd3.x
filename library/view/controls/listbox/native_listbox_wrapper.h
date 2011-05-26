
#ifndef __view_native_listbox_wrapper_h__
#define __view_native_listbox_wrapper_h__

#pragma once

namespace gfx
{
    class Size;
}

namespace view
{

    class Listbox;
    class View;

    // An interface implemented by an object that provides a platform-native
    // listbox.
    class NativeListboxWrapper
    {
    public:
        // Updates the listbox's content from its model.
        virtual void UpdateFromModel() = 0;

        // Updates the enabled state of the listbox from the associated view.
        virtual void UpdateEnabled() = 0;

        // Returns the number of rows in the table.
        virtual int GetRowCount() const = 0;

        // Returns the 0-based index of the currently selected row.
        virtual int SelectedRow() const = 0;

        // Selects the specified row, making sure it's visible.
        virtual void SelectRow(int row) = 0;

        // Returns the preferred size of the combobox.
        virtual gfx::Size GetPreferredSize() = 0;

        // Retrieves the views::View that hosts the native control.
        virtual View* GetView() = 0;

        // Sets the focus to the listbox.
        virtual void SetFocus() = 0;

        // Returns a handle to the underlying native view for testing.
        virtual HWND GetTestingHandle() const = 0;

        // Creates an appropriate NativeListboxWrapper for the platform.
        static NativeListboxWrapper* CreateNativeWrapper(Listbox* listbox);

    protected:
        virtual ~NativeListboxWrapper() {}
    };

} //namespace view

#endif //__view_native_listbox_wrapper_h__