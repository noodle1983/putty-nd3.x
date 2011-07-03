
#ifndef __view_menu_host_win_h__
#define __view_menu_host_win_h__

#pragma once

#include "native_menu_host.h"
#include "view/widget/native_widget_win.h"

namespace view
{

    namespace internal
    {
        class NativeMenuHostDelegate;
    }

    // MenuHost implementation for windows.
    class MenuHostWin : public NativeWidgetWin, public NativeMenuHost
    {
    public:
        explicit MenuHostWin(internal::NativeMenuHostDelegate* delegate);
        virtual ~MenuHostWin();

    private:
        // Overridden from NativeMenuHost:
        virtual void StartCapturing();
        virtual NativeWidget* AsNativeWidget();

        // Overridden from WidgetWin:
        virtual void OnDestroy();
        virtual void OnCancelMode();

        internal::NativeMenuHostDelegate* delegate_;

        DISALLOW_COPY_AND_ASSIGN(MenuHostWin);
    };

} //namespace view

#endif //__view_menu_host_win_h__