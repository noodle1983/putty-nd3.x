
#ifndef __wan_chrome_ui_browser_window_h__
#define __wan_chrome_ui_browser_window_h__

#pragma once

#include <windows.h>

class Browser;
class Profile;

namespace gfx
{
    class Rect;
}

////////////////////////////////////////////////////////////////////////////////
// BrowserWindow interface
//  An interface implemented by the "view" of the Browser window.
//
// NOTE: All getters may return NULL.
class BrowserWindow
{
public:
    // Show the window, or activates it if it's already visible.
    virtual void Show() = 0;

    // Sets the window's size and position to the specified values.
    virtual void SetBounds(const gfx::Rect& bounds) = 0;

    // Closes the frame as soon as possible.  If the frame is not in a drag
    // session, it will close immediately; otherwise, it will move offscreen (so
    // events are still fired) until the drag ends, then close. This assumes
    // that the Browser is not immediately destroyed, but will be eventually
    // destroyed by other means (eg, the tab strip going to zero elements).
    // Bad things happen if the Browser dtor is called directly as a result of
    // invoking this method.
    virtual void Close() = 0;

    // Activates (brings to front) the window. Restores the window from minimized
    // state if necessary.
    virtual void Activate() = 0;

    // Deactivates the window, making the next window in the Z order the active
    // window.
    virtual void Deactivate() = 0;

    // Returns true if the window is currently the active/focused window.
    virtual bool IsActive() const = 0;

    // Flashes the taskbar item associated with this frame.
    virtual void FlashFrame() = 0;

    // Return a platform dependent identifier for this frame. On Windows, this
    // returns an HWND.
    virtual HWND GetNativeHandle() = 0;

    // Inform the frame that the selected tab favicon or title has changed. Some
    // frames may need to refresh their title bar.
    virtual void UpdateTitleBar() = 0;

    // Returns the nonmaximized bounds of the frame (even if the frame is
    // currently maximized or minimized) in terms of the screen coordinates.
    virtual gfx::Rect GetRestoredBounds() const = 0;

    // Retrieves the window's current bounds, including its frame.
    // This will only differ from GetRestoredBounds() for maximized
    // and minimized windows.
    virtual gfx::Rect GetBounds() const = 0;

    // TODO(beng): REMOVE?
    // Returns true if the frame is maximized (aka zoomed).
    virtual bool IsMaximized() const = 0;

    // Accessors for fullscreen mode state.
    virtual void SetFullscreen(bool fullscreen) = 0;
    virtual bool IsFullscreen() const = 0;

    // Returns true if the fullscreen bubble is visible.
    virtual bool IsFullscreenBubbleVisible() const = 0;

    // Focuses the app menu like it was a menu bar.
    //
    // Not used on the Mac, which has a "normal" menu bar.
    virtual void FocusAppMenu() = 0;

    // Moves keyboard focus to the next pane.
    virtual void RotatePaneFocus(bool forwards) = 0;

    // Returns whether the tool bar is visible or not.
    virtual bool IsToolbarVisible() const = 0;

    // Tells the frame not to render as inactive until the next activation change.
    // This is required on Windows when dropdown selects are shown to prevent the
    // select from deactivating the browser frame. A stub implementation is
    // provided here since the functionality is Windows-specific.
    virtual void DisableInactiveFrame() {}

    // BrowserThemeProvider calls this when a user has changed his or her theme,
    // indicating that it's time to redraw everything.
    virtual void UserChangedTheme() = 0;

    // Shows the app menu (for accessibility).
    virtual void ShowAppMenu() = 0;

    // Construct a BrowserWindow implementation for the specified |browser|.
    static BrowserWindow* CreateBrowserWindow(Browser* browser);

protected:
    friend class BrowserView;
    virtual void DestroyBrowser() = 0;

    virtual ~BrowserWindow() {}
};

#endif //__wan_chrome_ui_browser_window_h__