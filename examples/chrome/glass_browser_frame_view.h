
#ifndef __glass_browser_frame_view_h__
#define __glass_browser_frame_view_h__

#pragma once

#include "base/memory/scoped_ptr.h"

#include "view/controls/button/button.h"
#include "view/window/non_client_view.h"

#include "browser_frame_win.h"
#include "browser_non_client_frame_view.h"

class BrowserView;
class SkBitmap;

class GlassBrowserFrameView : public BrowserNonClientFrameView
{
public:
    // Constructs a non-client view for an BrowserFrame.
    GlassBrowserFrameView(BrowserFrame* frame, BrowserView* browser_view);
    virtual ~GlassBrowserFrameView();

    // Overridden from BrowserNonClientFrameView:
    virtual gfx::Rect GetBoundsForTabStrip(view::View* tabstrip) const OVERRIDE;
    virtual int GetHorizontalTabStripVerticalOffset(bool restored) const OVERRIDE;
    virtual void UpdateThrobber(bool running) OVERRIDE;
    virtual gfx::Size GetMinimumSize() OVERRIDE;

    // Overridden from view::NonClientFrameView:
    virtual gfx::Rect GetBoundsForClientView() const OVERRIDE;
    virtual gfx::Rect GetWindowBoundsForClientBounds(
        const gfx::Rect& client_bounds) const OVERRIDE;
    virtual int NonClientHitTest(const gfx::Point& point) OVERRIDE;
    virtual void GetWindowMask(const gfx::Size& size, gfx::Path* window_mask)
        OVERRIDE {}
    virtual void EnableClose(bool enable) OVERRIDE {}
    virtual void ResetWindowControls() OVERRIDE {}
    virtual void UpdateWindowIcon() OVERRIDE {}

protected:
    // Overridden from view::View:
    virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;
    virtual void Layout() OVERRIDE;
    virtual bool HitTest(const gfx::Point& l) const OVERRIDE;

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

    // Returns the insets of the client area.
    gfx::Insets GetClientAreaInsets() const;

    // Returns the bounds of the client area for the specified view size.
    gfx::Rect CalculateClientAreaBounds(int width, int height) const;

    // Starts/Stops the window throbber running.
    void StartThrobber();
    void StopThrobber();

    // Displays the next throbber frame.
    void DisplayNextThrobberFrame();

    // The frame that hosts this view.
    BrowserFrame* frame_;

    // The BrowserView hosted within this View.
    BrowserView* browser_view_;

    // The bounds of the ClientView.
    gfx::Rect client_view_bounds_;

    // Whether or not the window throbber is currently animating.
    bool throbber_running_;

    // The index of the current frame of the throbber animation.
    int throbber_frame_;

    static const int kThrobberIconCount = 24;
    static HICON throbber_icons_[kThrobberIconCount];
    static void InitThrobberIcons();

    DISALLOW_COPY_AND_ASSIGN(GlassBrowserFrameView);
};

#endif //__glass_browser_frame_view_h__