
#ifndef __view_view_delegate_h__
#define __view_view_delegate_h__

#pragma once

#include <windows.h>

#include <string>

#include "../accessibility/accessibility_types.h"

namespace gfx
{
    class Rect;
}

class Clipboard;

namespace view
{

    class View;
    class Window;

    // ViewDelegate is an interface implemented by an object using the views
    // framework. It is used to obtain various high level application utilities
    // and perform some actions such as window placement saving.
    //
    // The embedding app must set view_delegate to assign its ViewDelegate
    // implementation.
    class ViewDelegate
    {
    public:
        virtual ~ViewDelegate() {}

        // Gets the clipboard.
        virtual Clipboard* GetClipboard() const = 0;

        // Saves the position, size and maximized state for the window with the
        // specified name.
        virtual void SaveWindowPlacement(Window* window,
            const std::wstring& window_name,
            const gfx::Rect& bounds,
            bool maximized) = 0;

        // Retrieves the saved position and size for the window with the specified
        // name.
        virtual bool GetSavedWindowBounds(Window* window,
            const std::wstring& window_name,
            gfx::Rect* bounds) const = 0;

        // Retrieves the saved maximized state for the window with the specified
        // name.
        virtual bool GetSavedMaximizedState(view::Window* window,
            const std::wstring& window_name,
            bool* maximized) const = 0;

        // Notify the delegate that an accessibility event has happened in
        // a particular view.
        virtual void NotifyAccessibilityEvent(View* view,
            AccessibilityTypes::Event event_type) = 0;

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

        // The active ViewDelegate used by the views system.
        static ViewDelegate* view_delegate;
    };

} //namespace view

#endif //__view_view_delegate_h__