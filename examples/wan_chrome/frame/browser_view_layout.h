
#ifndef __wan_chrome_frame_browser_view_layout_h__
#define __wan_chrome_frame_browser_view_layout_h__

#pragma once

#include "base/basic_types.h"

#include "view/layout/layout_manager.h"

namespace gfx
{
    class Point;
    class Size;
}

class BrowserView;

// The layout manager used in chrome browser.
class BrowserViewLayout : public view::LayoutManager
{
public:
    BrowserViewLayout();
    virtual ~BrowserViewLayout();

    // Returns the minimum size of the browser view.
    virtual gfx::Size GetMinimumSize();

    // Tests to see if the specified |point| (in nonclient view's coordinates)
    // is within the view managed by the laymanager. Returns one of
    // HitTestCompat enum defined in view/window/hit_test.h.
    // See also ClientView::NonClientHitTest.
    virtual int NonClientHitTest(const gfx::Point& point);

    // view::LayoutManager overrides:
    virtual void Installed(view::View* host);
    virtual void Uninstalled(view::View* host);
    virtual void ViewAdded(view::View* host, view::View* view);
    virtual void ViewRemoved(view::View* host, view::View* view);
    virtual void Layout(view::View* host);
    virtual gfx::Size GetPreferredSize(view::View* host);

protected:
    // Child view that the layout manager manages.
    view::View* contents_container_;

    BrowserView* browser_view_;

    DISALLOW_COPY_AND_ASSIGN(BrowserViewLayout);
};

#endif //__wan_chrome_frame_browser_view_layout_h__