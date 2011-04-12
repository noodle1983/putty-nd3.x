
#ifndef __view_menu_host_win_h__
#define __view_menu_host_win_h__

#pragma once

#include "../../widget/widget_win.h"
#include "menu_host.h"

namespace view
{

    class SubmenuView;

    // MenuHost implementation for windows.
    class MenuHostWin : public WidgetWin, public MenuHost
    {
    public:
        explicit MenuHostWin(SubmenuView* submenu);
        virtual ~MenuHostWin();

        // MenuHost overrides:
        virtual void InitMenuHost(HWND parent,
            const gfx::Rect& bounds,
            View* contents_view,
            bool do_capture);
        virtual bool IsMenuHostVisible();
        virtual void ShowMenuHost(bool do_capture);
        virtual void HideMenuHost();
        virtual void DestroyMenuHost();
        virtual void SetMenuHostBounds(const gfx::Rect& bounds);
        virtual void ReleaseMenuHostCapture();
        virtual HWND GetMenuHostWindow();

        // WidgetWin overrides:
        virtual void OnDestroy();
        virtual void OnCaptureChanged(HWND hwnd);
        virtual void OnCancelMode();

    protected:
        virtual RootView* CreateRootView();

        // Overriden to return false, we do NOT want to release capture
        // on mouse release.
        virtual bool ReleaseCaptureOnMouseReleased();

    private:
        void DoCapture();

        // If true, DestroyMenuHost has been invoked.
        bool destroying_;

        // If true, we own the capture and need to release it.
        bool owns_capture_;

        // The view we contain.
        SubmenuView* submenu_;

        DISALLOW_COPY_AND_ASSIGN(MenuHostWin);
    };

} //namespace view

#endif //__view_menu_host_win_h__