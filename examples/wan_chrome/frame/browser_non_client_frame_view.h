
#ifndef __wan_chrome_frame_browser_non_client_frame_view_h__
#define __wan_chrome_frame_browser_non_client_frame_view_h__

#pragma once

#include "view/view/non_client_view.h"

class BrowserFrame;
class BrowserView;

// A specialization of the NonClientFrameView object that provides additional
// Browser-specific methods.
class BrowserNonClientFrameView : public view::NonClientFrameView
{
public:
    BrowserNonClientFrameView() : NonClientFrameView() {}
    virtual ~BrowserNonClientFrameView() {}

    // Returns the bounds within which the TabStrip should be laid out.
    virtual gfx::Rect GetBoundsForTabStrip(view::View* tabstrip) const = 0;

    // Returns the y coordinate within the window at which the horizontal TabStrip
    // begins, or (in vertical tabs mode) would begin.  If |restored| is true,
    // this is calculated as if we were in restored mode regardless of the current
    // mode.  This is used to correctly align theme images.
    virtual int GetHorizontalTabStripVerticalOffset(bool restored) const = 0;
};

#endif //__wan_chrome_frame_browser_non_client_frame_view_h__