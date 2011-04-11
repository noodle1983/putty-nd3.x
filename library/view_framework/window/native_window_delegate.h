
#ifndef __view_framework_native_window_delegate_h__
#define __view_framework_native_window_delegate_h__

#pragma once

namespace view
{

    ////////////////////////////////////////////////////////////////////////////////
    // NativeWindowDelegate interface
    //
    //  An interface implemented by an object that receives notifications from a
    //  NativeWindow implementation.
    class NativeWindowDelegate
    {
    public:
        virtual ~NativeWindowDelegate() {}

        // Returns true if the window can be activated.
        virtual bool CanActivate() const = 0;

        virtual bool IsInactiveRenderingDisabled() const = 0;
        virtual void EnableInactiveRendering() = 0;

        // Returns true if the window is modal.
        virtual bool IsModal() const = 0;

        // Returns true if the window is a dialog box.
        virtual bool IsDialogBox() const = 0;

        // Returns true if the window is using a system native frame. Returns false if
        // it is rendering its own title bar, borders and controls.
        virtual bool IsUsingNativeFrame() const = 0;

        // Returns the smallest size the window can be resized to by the user.
        virtual gfx::Size GetMinimumSize() const = 0;

        // Returns the non-client component containing |point|, in client coordinates.
        virtual int GetNonClientComponent(const gfx::Point& point) const = 0;

        // Runs the specified native command. Returns true if the command is handled.
        virtual bool ExecuteCommand(int command_id) = 0;

        // Called just after the NativeWindow has been created.
        virtual void OnNativeWindowCreated(const gfx::Rect& bounds) = 0;

        // Called when the activation state of a window has changed.
        virtual void OnNativeWindowActivationChanged(bool active) = 0;

        // Called when the user begins/ends to change the bounds of the window.
        virtual void OnNativeWindowBeginUserBoundsChange() = 0;
        virtual void OnNativeWindowEndUserBoundsChange() = 0;

        // Called just before the native window is destroyed. This is the delegate's
        // last chance to do anything with the native window handle.
        virtual void OnNativeWindowDestroying() = 0;

        // Called just after the native window is destroyed.
        virtual void OnNativeWindowDestroyed() = 0;

        // Called when the native window's position or size has changed.
        virtual void OnNativeWindowBoundsChanged() = 0;
    };

} //namespace view

#endif //__view_framework_native_window_delegate_h__