
#ifndef __view_native_menu_host_delegate_h__
#define __view_native_menu_host_delegate_h__

#pragma once

namespace view
{

    class RootView;

    class NativeMenuHostDelegate
    {
    public:
        virtual ~NativeMenuHostDelegate() {}

        // Called when the NativeMenuHost is being destroyed.
        virtual void OnNativeMenuHostDestroy() = 0;

        // Called when the NativeMenuHost is losing input capture.
        virtual void OnNativeMenuHostCancelCapture() = 0;

        // Pass-thrus for Widget overrides.
        // TODO(beng): Remove once MenuHost is-a Widget.
        virtual RootView* CreateRootView() = 0;
        virtual bool ShouldReleaseCaptureOnMouseRelease() const = 0;
    };

} //namespace view

#endif //__view_native_menu_host_delegate_h__