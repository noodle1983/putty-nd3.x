
#ifndef __wan_chrome_frame_browser_frame_win_h__
#define __wan_chrome_frame_browser_frame_win_h__

#pragma once

#include "view/window/window_win.h"

#include "native_browser_frame.h"

class BrowserView;

class BrowserFrameWin : public view::WindowWin,
    public NativeBrowserFrame
{
public:
    static BrowserFrameWin* Create(BrowserView* browser_view);

    static const gfx::Font& GetTitleFont();

    BrowserFrameWin(BrowserView* browser_view);
    virtual ~BrowserFrameWin();

    void InitBrowserFrame();

    BrowserView* browser_view() const { return browser_view_; }

    gfx::Rect GetBoundsForTabStrip(view::View* tabstrip) const;

protected:
    // Overridden from view::WindowWin:
    virtual int GetShowState() const;
    virtual gfx::Insets GetClientAreaInsets() const;

    // Overridden from view::Window:
    virtual void Activate();
    virtual bool IsAppWindow() const { return true; }
    virtual void UpdateFrameAfterFrameChange();
    virtual view::RootView* CreateRootView();
    virtual view::NonClientFrameView* CreateFrameViewForWindow();

    // Overridden from NativeBrowserFrame:
    virtual view::NativeWindow* AsNativeWindow();
    virtual const view::NativeWindow* AsNativeWindow() const;
    virtual int GetMinimizeButtonOffset() const;
    virtual bool AlwaysUseNativeFrame() const;
    virtual void TabStripDisplayModeChanged();

private:
    friend class OpaqueBrowserFrameView;

    void UpdateDWMFrame();

    BrowserNonClientFrameView* browser_frame_view_;

    BrowserView* browser_view_;

    DISALLOW_COPY_AND_ASSIGN(BrowserFrameWin);
};

#endif //__wan_chrome_frame_browser_frame_win_h__