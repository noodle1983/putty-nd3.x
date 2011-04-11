
#ifndef __wan_chrome_ui_view_frame_glass_browser_frame_view_h__
#define __wan_chrome_ui_view_frame_glass_browser_frame_view_h__

#pragma once

#include "view_framework/controls/menu/view_menu_delegate.h"

#include "browser_non_client_frame_view.h"

class GlassBrowserFrameView : public BrowserNonClientFrameView,
    public view::ViewMenuDelegate
{
public:
    // Constructs a non-client view for an BrowserFrame.
    GlassBrowserFrameView(BrowserFrame* frame, BrowserView* browser_view);
    virtual ~GlassBrowserFrameView();

    // Overridden from BrowserNonClientFrameView:
    virtual gfx::Rect GetBoundsForTabStrip(view::View* tabstrip) const;
    virtual int GetHorizontalTabStripVerticalOffset(bool restored) const;
    virtual void UpdateThrobber(bool running);

    // Overridden from view::NonClientFrameView:
    virtual gfx::Rect GetBoundsForClientView() const;
    virtual bool AlwaysUseNativeFrame() const;
    virtual gfx::Rect GetWindowBoundsForClientBounds(
        const gfx::Rect& client_bounds) const;
    virtual int NonClientHitTest(const gfx::Point& point);
    virtual void GetWindowMask(const gfx::Size& size, gfx::Path* window_mask) {}
    virtual void EnableClose(bool enable) {}
    virtual void ResetWindowControls() {}
    virtual void UpdateWindowIcon() {}

    // view::ViewMenuDelegate implementation:
    virtual void RunMenu(view::View* source, const gfx::Point& pt);

protected:
    // Overridden from view::View:
    virtual void OnPaint(gfx::Canvas* canvas);
    virtual void Layout();
    virtual bool HitTest(const gfx::Point& l) const;

private:
    // Returns the thickness of the border that makes up the window frame edges.
    // This does not include any client edge.
    int FrameBorderThickness() const;

    // Returns the thickness of the entire nonclient left, right, and bottom
    // borders, including both the window frame and any client edge.
    int NonClientBorderThickness() const;

    // Returns the height of the entire nonclient top border, including the window
    // frame, any title area, and any connected client edge.  If |restored| is
    // true, acts as if the window is restored regardless of the real mode.  If
    // |ignore_vertical_tabs| is true, acts as if vertical tabs are off regardless
    // of the real state.
    int NonClientTopBorderHeight(bool restored, bool ignore_vertical_tabs) const;

    // Paint various sub-components of this view.
    void PaintToolbarBackground(gfx::Canvas* canvas);
    void PaintRestoredClientEdge(gfx::Canvas* canvas);

    // Layout various sub-components of this view.
    void LayoutClientView();

    // Returns the bounds of the client area for the specified view size.
    gfx::Rect CalculateClientAreaBounds(int width, int height) const;

    // The frame that hosts this view.
    BrowserFrame* frame_;

    // The BrowserView hosted within this View.
    BrowserView* browser_view_;

    // The bounds of the ClientView.
    gfx::Rect client_view_bounds_;

    DISALLOW_COPY_AND_ASSIGN(GlassBrowserFrameView);
};

#endif //__wan_chrome_ui_view_frame_glass_browser_frame_view_h__