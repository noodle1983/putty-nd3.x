
#ifndef __view_native_combobox_wrapper_h__
#define __view_native_combobox_wrapper_h__

#pragma once

#include <windows.h>

namespace gfx
{
    class Size;
}

namespace view
{

    class Combobox;
    class KeyEvent;
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

        // Retrieves the view::View that hosts the native control.
        virtual View* GetView() = 0;

        // Sets the focus to the combobox.
        virtual void SetFocus() = 0;

        // Invoked when a key is pressed/release on Combobox.  Subclasser
        // should return true if the event has been processed and false
        // otherwise.
        // See also View::OnKeyPressed/OnKeyReleased.
        virtual bool HandleKeyPressed(const KeyEvent& e) = 0;
        virtual bool HandleKeyReleased(const KeyEvent& e) = 0;

        // Invoked when focus is being moved from or to the Combobox.
        // See also View::OnFocus/OnBlur.
        virtual void HandleFocus() = 0;
        virtual void HandleBlur() = 0;

        // Returns a handle to the underlying native view for testing.
        virtual HWND GetTestingHandle() const = 0;

        static NativeComboboxWrapper* CreateWrapper(Combobox* combobox);

    protected:
        virtual ~NativeComboboxWrapper() {}
    };

} //namespace view

#endif //__view_native_combobox_wrapper_h__