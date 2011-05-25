
#ifndef __view_native_combobox_wrapper_h__
#define __view_native_combobox_wrapper_h__

#pragma once

namespace gfx
{
    class Size;
}

namespace view
{

    class Combobox;
    class View;

    class NativeComboboxWrapper
    {
    public:
        // Updates the combobox's content from its model.
        virtual void UpdateFromModel() = 0;

        // Updates the displayed selected item from the associated Combobox.
        virtual void UpdateSelectedItem() = 0;

        // Updates the enabled state of the combobox from the associated view.
        virtual void UpdateEnabled() = 0;

        // Gets the selected index.
        virtual int GetSelectedItem() const = 0;

        // Returns true if the Combobox dropdown is open.
        virtual bool IsDropdownOpen() const = 0;

        // Returns the preferred size of the combobox.
        virtual gfx::Size GetPreferredSize() = 0;

        // Retrieves the views::View that hosts the native control.
        virtual View* GetView() = 0;

        // Sets the focus to the combobox.
        virtual void SetFocus() = 0;

        // Returns a handle to the underlying native view for testing.
        virtual HWND GetTestingHandle() const = 0;

        static NativeComboboxWrapper* CreateWrapper(Combobox* combobox);

    protected:
        virtual ~NativeComboboxWrapper() {}
    };

} //namespace view

#endif //__view_native_combobox_wrapper_h__