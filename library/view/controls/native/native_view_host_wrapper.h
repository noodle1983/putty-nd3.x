
#ifndef __view_native_view_host_wrapper_h__
#define __view_native_view_host_wrapper_h__

#pragma once

namespace view
{

    class NativeViewHost;

    // An interface that implemented by an object that wraps a HWND on
    // a specific platform, used to perform platform specific operations on that
    // native view when attached, detached, moved and sized.
    class NativeViewHostWrapper
    {
    public:
        virtual ~NativeViewHostWrapper() {}

        // Called when a HWND has been attached to the associated
        // NativeViewHost, allowing the wrapper to perform platform-specific
        // initialization.
        virtual void NativeViewAttached() = 0;

        // Called before the attached HWND is detached from the
        // NativeViewHost, allowing the wrapper to perform platform-specific
        // cleanup. |destroyed| is true if the native view is detached
        // because it's being destroyed, or false otherwise.
        virtual void NativeViewDetaching(bool destroyed) = 0;

        // Called when our associated NativeViewHost is added to a View hierarchy
        // rooted at a valid Widget.
        virtual void AddedToWidget() = 0;

        // Called when our associated NativeViewHost is removed from a View hierarchy
        // rooted at a valid Widget.
        virtual void RemovedFromWidget() = 0;

        // Installs a clip on the HWND.
        virtual void InstallClip(int x, int y, int w, int h) = 0;

        // Whether or not a clip has been installed on the wrapped HWND.
        virtual bool HasInstalledClip() = 0;

        // Removes the clip installed on the HWND by way of InstallClip.
        virtual void UninstallClip() = 0;

        // Shows the HWND at the specified position (relative to the parent
        // native view).
        virtual void ShowWidget(int x, int y, int w, int h) = 0;

        // Hides the HWND. NOTE: this may be invoked when the native view
        // is already hidden.
        virtual void HideWidget() = 0;

        // Sets focus to the HWND.
        virtual void SetFocus() = 0;

        // Creates a platform-specific instance of an object implementing this
        // interface.
        static NativeViewHostWrapper* CreateWrapper(NativeViewHost* host);
    };

} //namespace view

#endif //__view_native_view_host_wrapper_h__