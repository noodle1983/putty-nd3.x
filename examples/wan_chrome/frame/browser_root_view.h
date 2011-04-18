
#ifndef __wan_chrome_frame_browser_root_view_h__
#define __wan_chrome_frame_browser_root_view_h__

#pragma once

#include "view/view/root_view.h"

class BrowserView;

class BrowserRootView : public view::RootView
{
public:
    BrowserRootView(BrowserView* browser_view, view::Widget* widget);

private:
    BrowserView* browser_view_;

    DISALLOW_COPY_AND_ASSIGN(BrowserRootView);
};

#endif //__wan_chrome_frame_browser_root_view_h__