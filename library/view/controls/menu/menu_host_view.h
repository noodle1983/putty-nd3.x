
#ifndef __view_menu_host_view_h__
#define __view_menu_host_view_h__

#pragma once

#include "native_menu_host.h"
#include "view/widget/native_widget_views.h"

namespace view
{
    namespace internal
    {
        class NativeMenuHostDelegate;
    }

    // MenuHost implementation for views.
    class MenuHostView : public NativeWidgetViews,
        public NativeMenuHost
    {
    public:
        explicit MenuHostView(internal::NativeMenuHostDelegate* delegate);
        virtual ~MenuHostView();

    private:
        // Overridden from NativeMenuHost:
        virtual void StartCapturing();
        virtual NativeWidget* AsNativeWidget();

        // Overridden from NativeWidgetViews:
        virtual void InitNativeWidget(const Widget::InitParams& params);

        internal::NativeMenuHostDelegate* delegate_;

        DISALLOW_COPY_AND_ASSIGN(MenuHostView);
    };

} //namespace view

#endif //__view_menu_host_view_h__