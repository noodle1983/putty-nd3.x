
#ifndef __view_view_delegate_h__
#define __view_view_delegate_h__

#pragma once

#include <windows.h>

#include <string>

#include "ui_base/accessibility/accessibility_types.h"
#include "ui_base/ui_base_types.h"

namespace gfx
{
    class Rect;
}

namespace ui
{
    class Clipboard;
}

namespace view
{

    class View;
    class Widget;

    // ViewDelegate is an interface implemented by an object using the views
    // framework. It is used to obtain various high level application utilities
    // and perform some actions such as window placement saving.
    //
    // The embedding app must set views_delegate to assign its ViewDelegate
    // implementation.
    class ViewDelegate
    {
    public:
        virtual ~ViewDelegate() {}

        // Gets the clipboard.
        virtual ui::Clipboard* GetClipboard() const = 0;

        // Returns the View that all synthetic widgets created without a specified
        // parent will be parented to if they do not specify a parent in their
        // InitParams, or NULL if they should have no parent.
        // TODO(beng): perhaps this should be a Widget.
        virtual View* GetDefaultParentView() = 0;

        // Saves the position, size and "show" state for the window with the
        // specified name.
        virtual void SaveWindowPlacement(
            const Widget* widget,
            const std::wstring& window_name,
            const gfx::Rect& bounds,
            ui::WindowShowState show_state) = 0;

        // Retrieves the saved position and size and "show" state for the window with
        // the specified name.
        virtual bool GetSavedWindowPlacement(
            const std::wstring& window_name,
            gfx::Rect* bounds,
            ui::WindowShowState* show_state) const = 0;

        virtual void NotifyAccessibilityEvent(View* view,
            ui::AccessibilityTypes::Event event_type) = 0;

        // For accessibility, notify the delegate that a menu item was focused
        // so that alternate feedback (speech / magnified text) can be provided.
        virtual void NotifyMenuItemFocused(
            const std::wstring& menu_name,
            const std::wstring& menu_item_name,
            int item_index,
            int item_count,
            bool has_submenu) = 0;

        // Retrieves the default window icon to use for windows if none is specified.
        virtual HICON GetDefaultWindowIcon() const = 0;

        // AddRef/ReleaseRef are invoked while a menu is visible. They are used to
        // ensure we don't attempt to exit while a menu is showing.
        virtual void AddRef() = 0;
        virtual void ReleaseRef() = 0;

        // Converts view::Event::flags to a WindowOpenDisposition.
        virtual int GetDispositionForEvent(int event_flags) = 0;

        // The active ViewDelegate used by the views system.
        static ViewDelegate* view_delegate;
    };

} //namespace view

#endif //__view_view_delegate_h__