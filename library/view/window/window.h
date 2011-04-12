
#ifndef __view_window_h__
#define __view_window_h__

#pragma once

#include "../view/non_client_view.h"
#include "native_window.h"
#include "native_window_delegate.h"

namespace gfx
{
    class Font;
    class Rect;
    class Size;
}

namespace view
{

    class NonClientFrameView;
    class Widget;
    class WindowDelegate;

    // An interface implemented by an object that provides a top level window.
    class Window : public NativeWindowDelegate
    {
    public:
        explicit Window(WindowDelegate* window_delegate);
        virtual ~Window();

        // Creates an instance of an object implementing this interface.
        // TODO(beng): create a version of this function that takes a HWND, for
        //             constrained windows.
        static Window* CreateWanWindow(HWND parent, const gfx::Rect& bounds,
            WindowDelegate* window_delegate);

        // Returns the preferred size of the contents view of this window based on
        // its localized size data. The width in cols is held in a localized string
        // resource identified by |col_resource_id|, the height in the same fashion.
        // TODO(beng): This should eventually live somewhere else, probably closer to
        //             ClientView.
        static int GetLocalizedContentsWidth(int col_resource_id);
        static int GetLocalizedContentsHeight(int row_resource_id);
        static gfx::Size GetLocalizedContentsSize(int col_resource_id,
            int row_resource_id);

        // Closes all windows that aren't identified as "app windows" via
        // IsAppWindow. Called during application shutdown when the last "app window"
        // is closed.
        static void CloseAllSecondaryWindows();

        // Used by |CloseAllSecondaryWindows|. If |widget|'s window is a secondary
        // window, the window is closed. If |widget| has no window, it is closed.
        // Does nothing if |widget| is null.
        static void CloseSecondaryWidget(Widget* widget);

        // Retrieves the window's bounds, including its frame.
        gfx::Rect GetBounds() const;

        // Retrieves the restored bounds for the window.
        gfx::Rect GetNormalBounds() const;

        // Sets the Window's bounds. The window is inserted after |other_window| in
        // the window Z-order. If this window is not yet visible, other_window's
        // monitor is used as the constraining rectangle, rather than this window's
        // monitor.
        void SetWindowBounds(const gfx::Rect& bounds, HWND other_window);

        // Makes the window visible.
        void Show();

        // Hides the window. This does not delete the window, it just hides it. This
        // always hides the window, it is separate from the stack maintained by
        // Push/PopForceHidden.
        virtual void HideWindow();

        // Prevents the window from being rendered as deactivated the next time it is.
        // This state is reset automatically as soon as the window becomes activated
        // again. There is no ability to control the state through this API as this
        // leads to sync problems.
        void DisableInactiveRendering();

        // Activates the window, assuming it already exists and is visible.
        void Activate();

        // Deactivates the window, making the next window in the Z order the active
        // window.
        void Deactivate();

        // Closes the window, ultimately destroying it. The window hides immediately,
        // and is destroyed after a return to the message loop. Close() can be called
        // multiple times.
        void CloseWindow();

        // Maximizes/minimizes/restores the window.
        void Maximize();
        void Minimize();
        void Restore();

        // Whether or not the window is currently active.
        bool IsActive() const;

        // Whether or not the window is currently visible.
        bool IsVisible() const;

        // Whether or not the window is maximized or minimized.
        bool IsMaximized() const;
        bool IsMinimized() const;

        // Accessors for fullscreen state.
        void SetFullscreen(bool fullscreen);
        bool IsFullscreen() const;

        // Sets whether or not the window should show its frame as a "transient drag
        // frame" - slightly transparent and without the standard window controls.
        void SetUseDragFrame(bool use_drag_frame);

        // Returns true if the Window is considered to be an "app window" - i.e.
        // any window which when it is the last of its type closed causes the
        // application to exit.
        virtual bool IsAppWindow() const;

        // Toggles the enable state for the Close button (and the Close menu item in
        // the system menu).
        void EnableClose(bool enable);

        // Tell the window to update its title from the delegate.
        void UpdateWindowTitle();

        // Tell the window to update its icon from the delegate.
        void UpdateWindowIcon();

        // Sets whether or not the window is always-on-top.
        void SetIsAlwaysOnTop(bool always_on_top);

        // Creates an appropriate NonClientFrameView for this window.
        virtual NonClientFrameView* CreateFrameViewForWindow();

        // Updates the frame after an event caused it to be changed.
        virtual void UpdateFrameAfterFrameChange();

        // Retrieves the Window's native window handle.
        HWND GetNativeWindow() const;

        // Whether we should be using a native frame.
        bool ShouldUseNativeFrame() const;

        // Tell the window that something caused the frame type to change.
        void FrameTypeChanged();

        // TODO(beng): remove once Window subclasses Widget.
        Widget* AsWidget();
        const Widget* AsWidget() const;

        WindowDelegate* window_delegate()
        {
            return const_cast<WindowDelegate*>(
                const_cast<const Window*>(this)->window_delegate());
        }
        const WindowDelegate* window_delegate() const
        {
            return window_delegate_;
        }

        NonClientView* non_client_view()
        {
            return const_cast<NonClientView*>(
                const_cast<const Window*>(this)->non_client_view());
        }
        const NonClientView* non_client_view() const
        {
            return non_client_view_;
        }

        ClientView* client_view()
        {
            return const_cast<ClientView*>(
                const_cast<const Window*>(this)->client_view());
        }
        const ClientView* client_view() const
        {
            return non_client_view()->client_view();
        }

        NativeWindow* native_window() { return native_window_; }

    protected:
        // TODO(beng): Temporarily provided as a way to associate the subclass'
        //             implementation of NativeWidget with this.
        void set_native_window(NativeWindow* native_window)
        {
            native_window_ = native_window;
        }

        // Overridden from NativeWindowDelegate:
        virtual bool CanActivate() const;
        virtual bool IsInactiveRenderingDisabled() const;
        virtual void EnableInactiveRendering();
        virtual bool IsModal() const;
        virtual bool IsDialogBox() const;
        virtual bool IsUsingNativeFrame() const;
        virtual gfx::Size GetMinimumSize() const;
        virtual int GetNonClientComponent(const gfx::Point& point) const;
        virtual bool ExecuteCommand(int command_id);
        virtual void OnNativeWindowCreated(const gfx::Rect& bounds);
        virtual void OnNativeWindowActivationChanged(bool active);
        virtual void OnNativeWindowBeginUserBoundsChange();
        virtual void OnNativeWindowEndUserBoundsChange();
        virtual void OnNativeWindowDestroying();
        virtual void OnNativeWindowDestroyed();
        virtual void OnNativeWindowBoundsChanged();

    private:
        Window();

        // Sizes and positions the window just after it is created.
        void SetInitialBounds(const gfx::Rect& bounds);

        // Persists the window's restored position and maximized state using the
        // window delegate.
        void SaveWindowPosition();

        NativeWindow* native_window_;

        // Our window delegate (see InitWindow() method for documentation).
        WindowDelegate* window_delegate_;

        // The View that provides the non-client area of the window (title bar,
        // window controls, sizing borders etc). To use an implementation other than
        // the default, this class must be sub-classed and this value set to the
        // desired implementation before calling |InitWindow()|.
        NonClientView* non_client_view_;

        // The saved maximized state for this window. See note in SetInitialBounds
        // that explains why we save this.
        bool saved_maximized_state_;

        // The smallest size the window can be.
        gfx::Size minimum_size_;

        // True when the window should be rendered as active, regardless of whether
        // or not it actually is.
        bool disable_inactive_rendering_;

        // Set to true if the window is in the process of closing .
        bool window_closed_;

        DISALLOW_COPY_AND_ASSIGN(Window);
    };

} //namespace view

#endif //__view_window_h__