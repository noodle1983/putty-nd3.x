
#ifndef __view_native_widget_h__
#define __view_native_widget_h__

#pragma once

#include "widget.h"

namespace view
{

    namespace internal
    {
        class NativeWidgetPrivate;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeWidget interface
    //
    //  An interface that serves as the public API base for the
    //  internal::NativeWidget interface that Widget uses to communicate with a
    //  backend-specific native widget implementation. This is the only component of
    //  this interface that is publicly visible, and exists solely for exposure via
    //  Widget's native_widget() accessor, which code occasionally static_casts to
    //  a known implementation in platform-specific code.
    class NativeWidget
    {
    public:
        virtual ~NativeWidget() {}

    private:
        friend class Widget;

        virtual internal::NativeWidgetPrivate* AsNativeWidgetPrivate() = 0;
    };

} //namespace view

#endif //__view_native_widget_h__