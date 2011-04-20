
#ifndef __wan_chrome_frame_browser_non_client_frame_view_h__
#define __wan_chrome_frame_browser_non_client_frame_view_h__

#pragma once

#include "view/view/non_client_view.h"

class BrowserNonClientFrameView : public view::NonClientFrameView
{
public:
    BrowserNonClientFrameView() : NonClientFrameView() {}
    virtual ~BrowserNonClientFrameView() {}

    virtual gfx::Rect GetBoundsForTabStrip(view::View* tabstrip) const = 0;

    virtual int GetHorizontalTabStripVerticalOffset(bool restored) const = 0;
};

#endif //__wan_chrome_frame_browser_non_client_frame_view_h__