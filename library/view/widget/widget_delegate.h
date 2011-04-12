
#ifndef __view_framework_widget_delegate_h__
#define __view_framework_widget_delegate_h__

#pragma once

namespace view
{

    // WidgetDelegate interface
    // Handles events on Widgets in context-specific ways.
    class WidgetDelegate
    {
    public:
        virtual ~WidgetDelegate() {}

        // Called whenever the widget is activated or deactivated.
        // TODO(beng): This should be consolidated with
        //             WindowDelegate::OnWindowActivationChanged().
        virtual void OnWidgetActivated(bool active) {}

        // Called whenever the widget's position changes.
        virtual void OnWidgetMove() {}

        // Called with the display changes (color depth or resolution).
        virtual void OnDisplayChanged() {}

        // Called when the work area (the desktop area minus task bars,
        // menu bars, etc.) changes in size.
        virtual void OnWorkAreaChanged() {}
    };

} //namespace view

#endif //__view_framework_widget_delegate_h__