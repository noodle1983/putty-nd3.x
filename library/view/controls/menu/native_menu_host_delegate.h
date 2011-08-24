
#ifndef __view_native_menu_host_delegate_h__
#define __view_native_menu_host_delegate_h__

#pragma once

namespace view
{
    namespace internal
    {

        class NativeWidgetDelegate;

        class NativeMenuHostDelegate
        {
        public:
            virtual ~NativeMenuHostDelegate() {}

            // Called when the NativeMenuHost is being destroyed.
            virtual void OnNativeMenuHostDestroy() = 0;

            // Called when the NativeMenuHost is losing input capture.
            virtual void OnNativeMenuHostCancelCapture() = 0;

            virtual internal::NativeWidgetDelegate* AsNativeWidgetDelegate() = 0;
        };

    } //namespace internal
} //namespace view

#endif //__view_native_menu_host_delegate_h__