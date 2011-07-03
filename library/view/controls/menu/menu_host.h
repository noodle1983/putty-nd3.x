
#ifndef __view_menu_host_h__
#define __view_menu_host_h__

#pragma once

#include <windows.h>

#include "base/basic_types.h"

#include "ui_gfx/rect.h"

#include "native_menu_host_delegate.h"
#include "view/widget/widget.h"

namespace view
{

    class NativeMenuHost;
    class NativeWidget;
    class SubmenuView;
    class View;

    // SubmenuView uses a MenuHost to house the SubmenuView. MenuHost typically
    // extends the native Widget type, but is defined here for clarity of what
    // methods SubmenuView uses.
    //
    // SubmenuView owns the MenuHost. When SubmenuView is done with the MenuHost
    // |DestroyMenuHost| is invoked. The one exception to this is if the native
    // OS destroys the widget out from under us, in which case |MenuHostDestroyed|
    // is invoked back on the SubmenuView and the SubmenuView then drops references
    // to the MenuHost.
    class MenuHost : public Widget, public internal::NativeMenuHostDelegate
    {
    public:
        explicit MenuHost(SubmenuView* submenu);
        virtual ~MenuHost();

        // Initializes and shows the MenuHost.
        void InitMenuHost(HWND parent,
            const gfx::Rect& bounds,
            View* contents_view,
            bool do_capture);

        // Returns true if the menu host is visible.
        bool IsMenuHostVisible();

        // Shows the menu host. If |do_capture| is true the menu host should do a
        // mouse grab.
        void ShowMenuHost(bool do_capture);

        // Hides the menu host.
        void HideMenuHost();

        // Destroys and deletes the menu host.
        void DestroyMenuHost();

        // Sets the bounds of the menu host.
        void SetMenuHostBounds(const gfx::Rect& bounds);

        // Releases a mouse grab installed by |ShowMenuHost|.
        void ReleaseMenuHostCapture();

        // Returns the native window of the MenuHost.
        HWND GetMenuHostWindow();

    private:
        // Overridden from Widget:
        virtual internal::RootView* CreateRootView();
        virtual bool ShouldReleaseCaptureOnMouseReleased() const;

        // Overridden from NativeMenuHostDelegate:
        virtual void OnNativeMenuHostDestroy();
        virtual void OnNativeMenuHostCancelCapture();
        virtual internal::NativeWidgetDelegate* AsNativeWidgetDelegate();

        NativeMenuHost* native_menu_host_;

        // The view we contain.
        SubmenuView* submenu_;

        // If true, DestroyMenuHost has been invoked.
        bool destroying_;

        DISALLOW_COPY_AND_ASSIGN(MenuHost);
    };

} //namespace view

#endif //__view_menu_host_h__