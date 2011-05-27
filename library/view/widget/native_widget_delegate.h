
#ifndef __view_native_widget_delegate_h__
#define __view_native_widget_delegate_h__

#pragma once

namespace gfx
{
    class Canvas;
    class Size;
}

namespace view
{

    ////////////////////////////////////////////////////////////////////////////////
    // NativeWidgetDelegate
    //
    //  An interface implemented by the object that handles events sent by a
    //  NativeWidget implementation.
    class NativeWidgetDelegate
    {
    public:
        virtual ~NativeWidgetDelegate() {}

        // Called when native focus moves from one native view to another.
        virtual void OnNativeFocus(HWND focused_view) = 0;
        virtual void OnNativeBlur(HWND focused_view) = 0;

        // Called when the native widget is created.
        virtual void OnNativeWidgetCreated() = 0;

        // Called when the NativeWidget changed size to |new_size|.
        virtual void OnSizeChanged(const gfx::Size& new_size) = 0;

        // Returns true if the delegate has a FocusManager.
        virtual bool HasFocusManager() const = 0;

        // Paints the rootview in the canvas. This will also refresh the compositor
        // tree if necessary when accelerated painting is enabled.
        virtual void OnNativeWidgetPaint(gfx::Canvas* canvas) = 0;

        // MouseEvent handlers.
        virtual bool OnMouseEvent(const MouseEvent& event) = 0;
        virtual void OnMouseCaptureLost() = 0;

        // SetCursor handlers.
        virtual bool OnNativeSetCursor(HWND window, UINT hit_test, UINT message) = 0;
    };

} //namespace view

#endif //__view_native_widget_delegate_h__