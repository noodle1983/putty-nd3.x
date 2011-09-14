
#ifndef __browser_view_layout_h__
#define __browser_view_layout_h__

#pragma once

#include "base/basic_types.h"

#include "ui_gfx/rect.h"

#include "view/layout/layout_manager.h"

class AbstractTabStripView;
class BookmarkBarView;
class Browser;
class BrowserView;
class ContentsContainer;
class TabContentsContainer;
class ToolbarView;

namespace gfx
{
    class Point;
    class Size;
}

namespace view
{
    class SingleSplitView;
}

// The layout manager used in chrome browser.
class BrowserViewLayout : public view::LayoutManager
{
public:
    BrowserViewLayout();
    virtual ~BrowserViewLayout();

    // Returns the minimum size of the browser view.
    virtual gfx::Size GetMinimumSize();

    // Returns the bounding box for the find bar.
    virtual gfx::Rect GetFindBarBoundingBox() const;

    // Returns true if the specified point(BrowserView coordinates) is in
    // in the window caption area of the browser window.
    virtual bool IsPositionInWindowCaption(const gfx::Point& point);

    // Tests to see if the specified |point| (in nonclient view's coordinates)
    // is within the views managed by the laymanager. Returns one of
    // HitTestCompat enum defined in views/window/hit_test.h.
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
    Browser* browser();
    const Browser* browser() const;

    // Layout the tab strip region, returns the coordinate of the bottom of the
    // TabStrip, for laying out subsequent controls. This also lays out the
    // compact navigation and options bars if the browser is in compact navigation
    // mode.
    virtual int LayoutTabStripRegion();

    // Layout the following controls, starting at |top|, returns the coordinate
    // of the bottom of the control, for laying out the next control.
    virtual int LayoutToolbar(int top);
    virtual int LayoutBookmarkAndInfoBars(int top);
    int LayoutBookmarkBar(int top);
    int LayoutInfoBar(int top);

    // Updates |source|'s reserved contents rect by mapping BrowserView's
    // |browser_reserved_rect| into |future_source_bounds| taking into
    // account |source|'s |future_parent_offset| (offset is relative to
    // browser_view_).
    void UpdateReservedContentsRect(const gfx::Rect& browser_reserved_rect,
        TabContentsContainer* source,
        const gfx::Rect& future_source_bounds,
        const gfx::Point& future_parent_offset);

    // Layout the TabContents container, between the coordinates |top| and
    // |bottom|.
    void LayoutTabContents(int top, int bottom);

    // Returns the top margin to adjust the contents_container_ by. This is used
    // to make the bookmark bar and contents_container_ overlap so that the
    // preview contents hides the bookmark bar.
    int GetTopMarginForActiveContent();

    // Returns true if an infobar is showing.
    bool InfobarVisible() const;

    // See description above vertical_layout_rect_ for details.
    void set_vertical_layout_rect(const gfx::Rect& bounds)
    {
        vertical_layout_rect_ = bounds;
    }
    const gfx::Rect& vertical_layout_rect() const
    {
        return vertical_layout_rect_;
    }

    // Child views that the layout manager manages.
    AbstractTabStripView* tabstrip_;
    ToolbarView* toolbar_;
    view::SingleSplitView* contents_split_;
    ContentsContainer* contents_container_;
    view::View* infobar_container_;
    BookmarkBarView* active_bookmark_bar_;

    BrowserView* browser_view_;

    // The bounds within which the vertically-stacked contents of the BrowserView
    // should be laid out within. When the SideTabstrip is not visible, this is
    // just the local bounds of the BrowserView, otherwise it's the local bounds
    // of the BrowserView less the width of the SideTabstrip.
    gfx::Rect vertical_layout_rect_;

    // The distance the FindBar is from the top of the window, in pixels.
    int find_bar_y_;

    DISALLOW_COPY_AND_ASSIGN(BrowserViewLayout);
};

#endif //__browser_view_layout_h__