
#ifndef __view_menu_host_win_h__
#define __view_menu_host_win_h__

#pragma once

#include "../../widget/widget_win.h"
#include "native_menu_host.h"

namespace view
{

    class NativeMenuHostDelegate;

    // MenuHost implementation for windows.
    class MenuHostWin : public WidgetWin, public NativeMenuHost
    {
    public:
        explicit MenuHostWin(NativeMenuHostDelegate* delegate);
        virtual ~MenuHostWin();

    private:
        // Overridden from NativeMenuHost:
        virtual void InitMenuHost(HWND parent,
            const gfx::Rect& bounds);
        virtual void StartCapturing();
        virtual NativeWidget* AsNativeWidget();

        // Overridden from WidgetWin:
        virtual void OnDestroy();
        virtual void OnCancelMode();
        virtual RootView* CreateRootView();
        virtual bool ShouldReleaseCaptureOnMouseReleased() const;

        scoped_ptr<NativeMenuHostDelegate> delegate_;

        DISALLOW_COPY_AND_ASSIGN(MenuHostWin);
    };

} //namespace view

#endif //__view_menu_host_win_h__