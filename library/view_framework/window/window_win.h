
#ifndef __view_framework_window_win_h__
#define __view_framework_window_win_h__

#pragma once

#include "../widget/widget_win.h"
#include "window.h"

namespace gfx
{
    class Font;
    class Point;
    class Size;
};

namespace view
{

    class Client;
    class WindowDelegate;

    ///////////////////////////////////////////////////////////////////////////////
    //
    // WindowWin
    //
    //  A WindowWin is a WidgetWin that has a caption and a border. The frame is
    //  rendered by the operating system.
    //
    ///////////////////////////////////////////////////////////////////////////////
    class WindowWin : public WidgetWin, public NativeWindow, public Window
    {
    public:
        virtual ~WindowWin();

        // Show the window with the specified show command.
        void Show(int show_state);

        // Accessors and setters for various properties.
        void set_focus_on_creation(bool focus_on_creation)
        {
            focus_on_creation_ = focus_on_creation;
        }

        // Hides the window if it hasn't already been force-hidden. The force hidden
        // count is tracked, so calling multiple times is allowed, you just have to
        // be sure to call PopForceHidden the same number of times.
        void PushForceHidden();

        // Decrements the force hidden count, showing the window if we have reached
        // the top of the stack. See PushForceHidden.
        void PopForceHidden();

        // Returns the system set window title font.
        static gfx::Font GetWindowTitleFont();

    protected:
        friend class Window;

        // Constructs the WindowWin. |window_delegate| cannot be NULL.
        explicit WindowWin(WindowDelegate* window_delegate);

        // Create the Window.
        // If parent is NULL, this WindowWin is top level on the desktop.
        // If |bounds| is empty, the view is queried for its preferred size and
        // centered on screen.
        virtual void Init(HWND parent, const gfx::Rect& bounds);

        // Returns the insets of the client area relative to the non-client area of
        // the window. Override this function instead of OnNCCalcSize, which is
        // crazily complicated.
        virtual gfx::Insets GetClientAreaInsets() const;

        // Retrieve the show state of the window. This is one of the SW_SHOW* flags
        // passed into Windows' ShowWindow method. For normal windows this defaults
        // to SW_SHOWNORMAL, however windows (e.g. the main window) can override this
        // method to provide different values (e.g. retrieve the user's specified
        // show state from the shortcut starutp info).
        virtual int GetShowState() const;

        // Overridden from WidgetWin:
        virtual void OnActivateApp(BOOL active, DWORD thread_id);
        virtual LRESULT OnAppCommand(HWND window, short app_command, WORD device,
            int keystate);
        virtual void OnClose();
        virtual void OnCommand(UINT notification_code, int command_id, HWND window);
        virtual void OnDestroy();
        virtual LRESULT OnDwmCompositionChanged(UINT msg, WPARAM w_param,
            LPARAM l_param);
        virtual void OnEnterSizeMove();
        virtual void OnExitSizeMove();
        virtual void OnFinalMessage(HWND window);
        virtual void OnGetMinMaxInfo(MINMAXINFO* minmax_info);
        virtual void OnInitMenu(HMENU menu);
        virtual LRESULT OnMouseActivate(UINT message, WPARAM w_param, LPARAM l_param);
        virtual LRESULT OnMouseRange(UINT message, WPARAM w_param, LPARAM l_param);
        virtual LRESULT OnNCActivate(BOOL active);
        LRESULT OnNCCalcSize(BOOL mode, LPARAM l_param); // ²»ÒªÔÙ¸²¸Ç.
        virtual LRESULT OnNCHitTest(const gfx::Point& point);
        virtual void OnNCPaint(HRGN rgn);
        virtual LRESULT OnNCMouseRange(UINT message, WPARAM w_param, LPARAM l_param);
        virtual LRESULT OnNCUAHDrawCaption(UINT msg, WPARAM w_param, LPARAM l_param);
        virtual LRESULT OnNCUAHDrawFrame(UINT msg, WPARAM w_param, LPARAM l_param);
        virtual LRESULT OnSetIcon(UINT size_type, HICON new_icon);
        virtual LRESULT OnSetText(const wchar_t* text);
        virtual void OnSettingChange(UINT flags, const wchar_t* section);
        virtual void OnSize(UINT size_param, const gfx::Size& new_size);
        virtual void OnSysCommand(UINT notification_code, gfx::Point click);
        virtual void OnWindowPosChanging(WINDOWPOS* window_pos);
        virtual Window* GetWindow() { return this; }
        virtual const Window* GetWindow() const { return this; }
        virtual void Close();

        // Overridden from NativeWindow:
        virtual NativeWidget* AsNativeWidget();
        virtual const NativeWidget* AsNativeWidget() const;
        virtual gfx::Rect GetRestoredBounds() const;
        virtual void ShowNativeWindow(ShowState state);
        virtual void BecomeModal();
        virtual void CenterWindow(const gfx::Size& size);
        virtual void GetWindowBoundsAndMaximizedState(gfx::Rect* bounds,
            bool* maximized) const;
        virtual void EnableClose(bool enable);
        virtual void SetWindowTitle(const std::wstring& title);
        virtual void SetWindowIcons(const SkBitmap& window_icon,
            const SkBitmap& app_icon);
        virtual void SetAccessibleName(const std::wstring& name);
        virtual void SetAccessibleRole(AccessibilityTypes::Role role);
        virtual void SetAccessibleState(AccessibilityTypes::State state);
        virtual void SetWindowBounds(const gfx::Rect& bounds, HWND other_window);
        virtual void HideWindow();
        virtual void Activate();
        virtual void Deactivate();
        virtual void Maximize();
        virtual void Minimize();
        virtual void Restore();
        virtual bool IsActive() const;
        virtual bool IsVisible() const;
        virtual bool IsMaximized() const;
        virtual bool IsMinimized() const;
        virtual void SetFullscreen(bool fullscreen);
        virtual bool IsFullscreen() const;
        virtual void SetAlwaysOnTop(bool always_on_top);
        virtual bool IsAppWindow() const;
        virtual void SetUseDragFrame(bool use_drag_frame);
        virtual NonClientFrameView* CreateFrameViewForWindow();
        virtual void UpdateFrameAfterFrameChange();
        virtual HWND GetNativeWindow() const;
        virtual bool ShouldUseNativeFrame() const;
        virtual void FrameTypeChanged();

    private:
        // Information saved before going into fullscreen mode, used to restore the
        // window afterwards.
        struct SavedWindowInfo
        {
            bool maximized;
            LONG style;
            LONG ex_style;
            RECT window_rect;
        };

        // Sets-up the focus manager with the view that should have focus when the
        // window is shown the first time.  If NULL is returned, the focus goes to the
        // button if there is one, otherwise the to the Cancel button.
        void SetInitialFocus();

        // If necessary, enables all ancestors.
        void RestoreEnabledIfNecessary();

        // Calculate the appropriate window styles for this window.
        DWORD CalculateWindowStyle();
        DWORD CalculateWindowExStyle();

        // Lock or unlock the window from being able to redraw itself in response to
        // updates to its invalid region.
        class ScopedRedrawLock;
        void LockUpdates();
        void UnlockUpdates();

        // Stops ignoring SetWindowPos() requests (see below).
        void StopIgnoringPosChanges() { ignore_window_pos_changes_ = false; }

        // Resets the window region for the current window bounds if necessary.
        // If |force| is true, the window region is reset to NULL even for native
        // frame windows.
        void ResetWindowRegion(bool force);

        //  Update accessibility information via our WindowDelegate.
        void UpdateAccessibleName(std::wstring& accessible_name);
        void UpdateAccessibleRole();
        void UpdateAccessibleState();

        // Calls the default WM_NCACTIVATE handler with the specified activation
        // value, safely wrapping the call in a ScopedRedrawLock to prevent frame
        // flicker.
        LRESULT CallDefaultNCActivateHandler(BOOL active);

        // Executes the specified SC_command.
        void ExecuteSystemMenuCommand(int command);

        // Static resource initialization.
        static void InitClass();
        enum ResizeCursor
        {
            RC_NORMAL = 0, RC_VERTICAL, RC_HORIZONTAL, RC_NESW, RC_NWSE
        };
        static HCURSOR resize_cursors_[6];

        // A delegate implementation that handles events received here.
        NativeWindowDelegate* delegate_;

        // Whether we should SetFocus() on a newly created window after
        // Init(). Defaults to true.
        bool focus_on_creation_;

        // Whether all ancestors have been enabled. This is only used if is_modal_ is
        // true.
        bool restored_enabled_;

        // True if we're in fullscreen mode.
        bool fullscreen_;

        // Saved window information from before entering fullscreen mode.
        SavedWindowInfo saved_window_info_;

        // True if this window is the active top level window.
        bool is_active_;

        // True if updates to this window are currently locked.
        bool lock_updates_;

        // The window styles of the window before updates were locked.
        DWORD saved_window_style_;

        // When true, this flag makes us discard incoming SetWindowPos() requests that
        // only change our position/size.  (We still allow changes to Z-order,
        // activation, etc.)
        bool ignore_window_pos_changes_;

        // The following factory is used to ignore SetWindowPos() calls for short time
        // periods.
        ScopedRunnableMethodFactory<WindowWin> ignore_pos_changes_factory_;

        // If this is greater than zero, we should prevent attempts to make the window
        // visible when we handle WM_WINDOWPOSCHANGING. Some calls like
        // ShowWindow(SW_RESTORE) make the window visible in addition to restoring it,
        // when all we want to do is restore it.
        int force_hidden_count_;

        // Set to true when the user presses the right mouse button on the caption
        // area. We need this so we can correctly show the context menu on mouse-up.
        bool is_right_mouse_pressed_on_caption_;

        // The last-seen monitor containing us, and its rect and work area.  These are
        // used to catch updates to the rect and work area and react accordingly.
        HMONITOR last_monitor_;
        gfx::Rect last_monitor_rect_, last_work_area_;

        // The window styles before we modified them for the drag frame appearance.
        DWORD drag_frame_saved_window_style_;
        DWORD drag_frame_saved_window_ex_style_;

        DISALLOW_COPY_AND_ASSIGN(WindowWin);
    };

} //namespace view

#endif  //__view_framework_window_win_h__