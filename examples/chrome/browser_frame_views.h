
#ifndef __browser_frame_views_h__
#define __browser_frame_views_h__

#pragma once

#include "base/basic_types.h"

#include "view/widget/native_widget_views.h"

#include "browser_frame.h"
#include "native_browser_frame.h"

class BrowserView;

////////////////////////////////////////////////////////////////////////////////
// BrowserFrameViews
//
//  BrowserFrameViews is a NativeWidgetViews subclass that provides the window
//  frame for the Chrome browser window.
//
class BrowserFrameViews : public view::NativeWidgetViews,
    public NativeBrowserFrame
{
public:
    BrowserFrameViews(BrowserFrame* browser_frame, BrowserView* browser_view);
    virtual ~BrowserFrameViews();

    BrowserView* browser_view() const { return browser_view_; }

protected:
    // Overridden from NativeBrowserFrame:
    virtual view::NativeWidget* AsNativeWidget() OVERRIDE;
    virtual const view::NativeWidget* AsNativeWidget() const OVERRIDE;
    virtual int GetMinimizeButtonOffset() const OVERRIDE;
    virtual void TabStripDisplayModeChanged() OVERRIDE;

private:
    // The BrowserView is our ClientView. This is a pointer to it.
    BrowserView* browser_view_;

    BrowserFrame* browser_frame_;

    DISALLOW_COPY_AND_ASSIGN(BrowserFrameViews);
};

#endif //__browser_frame_views_h__