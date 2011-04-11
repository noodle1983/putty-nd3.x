
#ifndef __wan_chrome_ui_view_frame_app_panel_browser_frame_view_h__
#define __wan_chrome_ui_view_frame_app_panel_browser_frame_view_h__

#pragma once

#include "view_framework/controls/button/button.h"

#include "browser_non_client_frame_view.h"

namespace gfx
{
    class Font;
}

namespace view
{
    class ImageButton;
    class ImageView;
}

// The frame view which is used for Application Panels.
// TODO(rafaelw): Refactor. This shares much duplicated code with
// OpaqueBrowserFrameView.
class AppPanelBrowserFrameView : public BrowserNonClientFrameView,
    public view::ButtonListener
{
public:
    // Constructs a non-client view for an BrowserFrame.
    AppPanelBrowserFrameView(BrowserFrame* frame, BrowserView* browser_view);
    virtual ~AppPanelBrowserFrameView();

    // Overridden from BrowserNonClientFrameView:
    virtual gfx::Rect GetBoundsForTabStrip(view::View* tabstrip) const;
    virtual int GetHorizontalTabStripVerticalOffset(bool restored) const;
    virtual void UpdateThrobber(bool running);
    virtual gfx::Size GetMinimumSize();

protected:
    // Overridden from view::NonClientFrameView:
    virtual gfx::Rect GetBoundsForClientView() const;
    virtual bool AlwaysUseCustomFrame() const;
    virtual bool AlwaysUseNativeFrame() const;
    virtual gfx::Rect GetWindowBoundsForClientBounds(
        const gfx::Rect& client_bounds) const;
    virtual int NonClientHitTest(const gfx::Point& point);
    virtual void GetWindowMask(const gfx::Size& size, gfx::Path* window_mask);
    virtual void EnableClose(bool enable);
    virtual void ResetWindowControls();
    virtual void UpdateWindowIcon();

    // Overridden from view::View:
    virtual void OnPaint(gfx::Canvas* canvas);
    virtual void Layout();

    // Overridden from view::ButtonListener:
    virtual void ButtonPressed(view::Button* sender, const view::Event& event);

private:
    // Returns the thickness of the border that makes up the window frame edges.
    // This does not include any client edge.
    int FrameBorderThickness() const;

    // Returns the thickness of the entire nonclient left, right, and bottom
    // borders, including both the window frame and any client edge.
    int NonClientBorderThickness() const;

    // Returns the height of the entire nonclient top border, including the window
    // frame, any title area, and any connected client edge.
    int NonClientTopBorderHeight() const;

    // Returns the thickness of the nonclient portion of the 3D edge along the
    // bottom of the titlebar.
    int TitlebarBottomThickness() const;

    // Returns the size of the titlebar icon.
    int IconSize() const;

    // Returns the bounds of the titlebar icon.
    gfx::Rect IconBounds() const;

    // Paint various sub-components of this view.  The *FrameBorder() function
    // also paints the background of the titlebar area, since the top frame border
    // and titlebar background are a contiguous component.
    void PaintRestoredFrameBorder(gfx::Canvas* canvas);
    void PaintMaximizedFrameBorder(gfx::Canvas* canvas);
    void PaintTitleBar(gfx::Canvas* canvas);
    void PaintRestoredClientEdge(gfx::Canvas* canvas);

    // Layout various sub-components of this view.
    void LayoutWindowControls();
    void LayoutTitleBar();

    // Returns the bounds of the client area for the specified view size.
    gfx::Rect CalculateClientAreaBounds(int width, int height) const;

    // The layout rect of the title, if visible.
    gfx::Rect title_bounds_;

    // Window controls.
    view::ImageButton* close_button_;

    // The frame that hosts this view.
    BrowserFrame* frame_;

    // The BrowserView hosted within this View.
    BrowserView* browser_view_;

    // The bounds of the ClientView.
    gfx::Rect client_view_bounds_;

    // The accessible name of this view.
    std::wstring accessible_name_;

    static void InitAppWindowResources();
    static gfx::Font* title_font_;

    DISALLOW_COPY_AND_ASSIGN(AppPanelBrowserFrameView);
};

#endif //__wan_chrome_ui_view_frame_app_panel_browser_frame_view_h__