
#ifndef __wan_chrome_ui_view_frame_browser_frame_win_h__
#define __wan_chrome_ui_view_frame_browser_frame_win_h__

#pragma once

#include "view_framework/window/window_win.h"

#include "browser_frame.h"
#include "native_browser_frame.h"

class BrowserView;
class Profile;

///////////////////////////////////////////////////////////////////////////////
// BrowserFrameWin
//
//  BrowserFrame is a WindowWin subclass that provides the window frame for the
//  Chrome browser window.
class BrowserFrameWin : public BrowserFrame,
    public view::WindowWin,
    public NativeBrowserFrame
{
public:
    // Normally you will create this class by calling BrowserFrame::Create.
    // Init must be called before using this class, which Create will do for you.
    BrowserFrameWin(BrowserView* browser_view, Profile* profile);
    virtual ~BrowserFrameWin();

    // This initialization function must be called after construction, it is
    // separate to avoid recursive calling of the frame from its constructor.
    void InitBrowserFrame();

    BrowserView* browser_view() const { return browser_view_; }

    // Explicitly sets how windows are shown. Use a value of -1 to give the
    // default behavior. This is used during testing and not generally useful
    // otherwise.
    static void SetShowState(int state);

protected:
    // Overridden from view::WindowWin:
    virtual int GetShowState() const;
    virtual gfx::Insets GetClientAreaInsets() const;
    virtual bool GetAccelerator(int cmd_id, view::MenuAccelerator* accelerator);
    virtual void OnEndSession(BOOL ending, UINT logoff);
    virtual void OnInitMenuPopup(HMENU menu, UINT position,
        BOOL is_system_menu);
    virtual void OnWindowPosChanged(WINDOWPOS* window_pos);
    virtual ThemeProvider* GetThemeProvider() const;
    virtual void OnScreenReaderDetected();

    // Overridden from view::Window:
    virtual void Activate();
    virtual bool IsAppWindow() const { return true; }
    virtual void UpdateFrameAfterFrameChange();
    virtual view::RootView* CreateRootView();
    virtual view::NonClientFrameView* CreateFrameViewForWindow();

    // Overridden from NativeBrowserFrame:
    virtual view::NativeWindow* AsNativeWindow();
    virtual const view::NativeWindow* AsNativeWindow() const;
    virtual BrowserNonClientFrameView* CreateBrowserNonClientFrameView();
    virtual int GetMinimizeButtonOffset() const;
    virtual ThemeProvider* GetThemeProviderForFrame() const;
    virtual bool AlwaysUseNativeFrame() const;
    virtual void TabStripDisplayModeChanged();

private:
    // Updates the DWM with the frame bounds.
    void UpdateDWMFrame();

    NativeBrowserFrameDelegate* delegate_;

    // The BrowserView is our ClientView. This is a pointer to it.
    BrowserView* browser_view_;

    DISALLOW_COPY_AND_ASSIGN(BrowserFrameWin);
};

#endif //__wan_chrome_ui_view_frame_browser_frame_win_h__