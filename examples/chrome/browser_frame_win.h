
#ifndef __browser_frame_win_h__
#define __browser_frame_win_h__

#pragma once

#include "base/basic_types.h"

#include "view/widget/native_widget_win.h"

#include "browser_frame.h"
#include "native_browser_frame.h"

class BrowserView;

////////////////////////////////////////////////////////////////////////////////
// BrowserFrameWin
//
//  BrowserFrameWin is a NativeWidgetWin subclass that provides the window frame
//  for the Chrome browser window.
//
class BrowserFrameWin : public view::NativeWidgetWin,
    public NativeBrowserFrame
{
public:
    BrowserFrameWin(BrowserFrame* browser_frame, BrowserView* browser_view);
    virtual ~BrowserFrameWin();

    BrowserView* browser_view() const { return browser_view_; }

    // Explicitly sets how windows are shown. Use a value of -1 to give the
    // default behavior. This is used during testing and not generally useful
    // otherwise.
    static void SetShowState(int state);

protected:
    // Overridden from view::NativeWidgetWin:
    virtual int GetShowState() const OVERRIDE;
    virtual gfx::Insets GetClientAreaInsets() const OVERRIDE;
    virtual void UpdateFrameAfterFrameChange() OVERRIDE;
    virtual void OnEndSession(BOOL ending, UINT logoff) OVERRIDE;
    virtual void OnInitMenuPopup(HMENU menu,
        UINT position,
        BOOL is_system_menu) OVERRIDE;
    virtual void OnWindowPosChanged(WINDOWPOS* window_pos) OVERRIDE;
    virtual void OnScreenReaderDetected() OVERRIDE;
    virtual bool ShouldUseNativeFrame() const OVERRIDE;

    // Overridden from NativeBrowserFrame:
    virtual view::NativeWidget* AsNativeWidget() OVERRIDE;
    virtual const view::NativeWidget* AsNativeWidget() const OVERRIDE;
    virtual int GetMinimizeButtonOffset() const OVERRIDE;
    virtual void TabStripDisplayModeChanged() OVERRIDE;

private:
    // Updates the DWM with the frame bounds.
    void UpdateDWMFrame();

    // The BrowserView is our ClientView. This is a pointer to it.
    BrowserView* browser_view_;

    BrowserFrame* browser_frame_;

    DISALLOW_COPY_AND_ASSIGN(BrowserFrameWin);
};

#endif //__browser_frame_win_h__