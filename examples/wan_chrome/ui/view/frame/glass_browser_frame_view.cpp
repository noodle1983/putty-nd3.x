
#include "glass_browser_frame_view.h"

#include "gfx/canvas.h"

#include "view/base/app_res_ids.h"
#include "view/gfx/theme_provider.h"
#include "view/window/window.h"

#include "../../../../wanui_res/resource.h"
#include "../../../theme/browser_theme_provider.h"
#include "browser_frame.h"
#include "browser_view.h"

namespace
{
    // There are 3 px of client edge drawn inside the outer frame borders.
    const int kNonClientBorderThickness = 3;
    // Vertical tabs have 4 px border.
    const int kNonClientVerticalTabStripBorderThickness = 4;
    // Besides the frame border, there's another 11 px of empty space atop the
    // window in restored mode, to use to drag the window around.
    const int kNonClientRestoredExtraThickness = 11;
    // In the window corners, the resize areas don't actually expand bigger, but the
    // 16 px at the end of the top and bottom edges triggers diagonal resizing.
    const int kResizeAreaCornerSize = 16;
    // The OTR avatar ends 2 px above the bottom of the tabstrip (which, given the
    // way the tabstrip draws its bottom edge, will appear like a 1 px gap to the
    // user).
    const int kOTRBottomSpacing = 2;
    // There are 2 px on each side of the OTR avatar (between the frame border and
    // it on the left, and between it and the tabstrip on the right).
    const int kOTRSideSpacing = 2;
    // The content left/right images have a shadow built into them.
    const int kContentEdgeShadowThickness = 2;
    // The top 1 px of the tabstrip is shadow; in maximized mode we push this off
    // the top of the screen so the tabs appear flush against the screen edge.
    const int kTabstripTopShadowThickness = 1;
    // In restored mode, the New Tab button isn't at the same height as the caption
    // buttons, but the space will look cluttered if it actually slides under them,
    // so we stop it when the gap between the two is down to 5 px.
    const int kNewTabCaptionRestoredSpacing = 5;
    // In maximized mode, where the New Tab button and the caption buttons are at
    // similar vertical coordinates, we need to reserve a larger, 16 px gap to avoid
    // looking too cluttered.
    const int kNewTabCaptionMaximizedSpacing = 16;
    // Menu should display below the profile button tag image on the frame. This
    // offset size depends on whether the frame is in glass or opaque mode.
    const int kMenuDisplayOffset = 7;
    // Y position for profile button inside the frame.
    const int kProfileButtonYPosition = 2;
    // Y position for profile tag inside the frame.
    const int kProfileTagYPosition = 1;
    // Offset y position of profile button and tag by this amount when maximized.
    const int kProfileElementMaximizedYOffset = 6;
}

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, public:

GlassBrowserFrameView::GlassBrowserFrameView(BrowserFrame* frame,
                                             BrowserView* browser_view)
                                             : BrowserNonClientFrameView(),
                                             frame_(frame),
                                             browser_view_(browser_view)
{
}

GlassBrowserFrameView::~GlassBrowserFrameView() {}

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, BrowserNonClientFrameView implementation:

gfx::Rect GlassBrowserFrameView::GetBoundsForTabStrip(
    view::View* tabstrip) const
{
    int minimize_button_offset =
        std::min(frame_->GetMinimizeButtonOffset(), width());
    int tabstrip_x = NonClientBorderThickness();
    // In RTL languages, we have moved an avatar icon left by the size of window
    // controls to prevent it from being rendered over them. So, we use its x
    // position to move this tab strip left when maximized. Also, we can render
    // a tab strip until the left end of this window without considering the size
    // of window controls in RTL languages.
    if(base::IsRTL())
    {
        minimize_button_offset = width();
    }
    int maximized_spacing = kNewTabCaptionMaximizedSpacing;
    int tabstrip_width = minimize_button_offset - tabstrip_x -
        (frame_->GetWindow()->IsMaximized() ?
            maximized_spacing : kNewTabCaptionRestoredSpacing);
    return gfx::Rect(tabstrip_x, GetHorizontalTabStripVerticalOffset(false),
        std::max(0, tabstrip_width), tabstrip->GetPreferredSize().height());
}

int GlassBrowserFrameView::GetHorizontalTabStripVerticalOffset(
    bool restored) const
{
    return NonClientTopBorderHeight(restored, true);
}

void GlassBrowserFrameView::UpdateThrobber(bool running)
{
}

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, view::NonClientFrameView implementation:

gfx::Rect GlassBrowserFrameView::GetBoundsForClientView() const
{
    return client_view_bounds_;
}

bool GlassBrowserFrameView::AlwaysUseNativeFrame() const
{
    return frame_->AlwaysUseNativeFrame();
}

gfx::Rect GlassBrowserFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const
{
    HWND hwnd = frame_->GetWindow()->GetNativeWindow();
    if(!browser_view_->IsTabStripVisible() && hwnd)
    {
        // If we don't have a tabstrip, we're either a popup or an app window, in
        // which case we have a standard size non-client area and can just use
        // AdjustWindowRectEx to obtain it. We check for a non-NULL window handle in
        // case this gets called before the window is actually created.
        RECT rect = client_bounds.ToRECT();
        AdjustWindowRectEx(&rect, GetWindowLong(hwnd, GWL_STYLE), FALSE,
            GetWindowLong(hwnd, GWL_EXSTYLE));
        return gfx::Rect(rect);
    }

    int top_height = NonClientTopBorderHeight(false, false);
    int border_thickness = NonClientBorderThickness();
    return gfx::Rect(std::max(0, client_bounds.x() - border_thickness),
        std::max(0, client_bounds.y() - top_height),
        client_bounds.width() + (2 * border_thickness),
        client_bounds.height() + top_height + border_thickness);
}

int GlassBrowserFrameView::NonClientHitTest(const gfx::Point& point)
{
    // If the browser isn't in normal mode, we haven't customized the frame, so
    // Windows can figure this out.  If the point isn't within our bounds, then
    // it's in the native portion of the frame, so again Windows can figure it
    // out.
    if(!browser_view_->IsBrowserTypeNormal() || !bounds().Contains(point))
    {
        return HTNOWHERE;
    }

    int frame_component =
        frame_->GetWindow()->client_view()->NonClientHitTest(point);

    // See if we're in the sysmenu region.  We still have to check the tabstrip
    // first so that clicks in a tab don't get treated as sysmenu clicks.
    int nonclient_border_thickness = NonClientBorderThickness();
    if(gfx::Rect(nonclient_border_thickness, GetSystemMetrics(SM_CXSIZEFRAME),
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON)).Contains(point))
    {
        return (frame_component == HTCLIENT) ? HTCLIENT : HTSYSMENU;
    }

    if(frame_component != HTNOWHERE)
    {
        return frame_component;
    }

    int frame_border_thickness = FrameBorderThickness();
    int window_component = GetHTComponentForFrame(point, frame_border_thickness,
        nonclient_border_thickness, frame_border_thickness,
        kResizeAreaCornerSize - frame_border_thickness,
        frame_->GetWindow()->window_delegate()->CanResize());
    // Fall back to the caption if no other component matches.
    return (window_component == HTNOWHERE) ? HTCAPTION : window_component;
}

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, view::ViewMenuDelegate implementation:
void GlassBrowserFrameView::RunMenu(view::View *source, const gfx::Point &pt)
{
}

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, view::View overrides:

void GlassBrowserFrameView::OnPaint(gfx::Canvas* canvas)
{
    if(!browser_view_->IsTabStripVisible())
    {
        return; // Nothing is visible, so don't bother to paint.
    }

    PaintToolbarBackground(canvas);
    if(!frame_->GetWindow()->IsMaximized())
    {
        PaintRestoredClientEdge(canvas);
    }
}

void GlassBrowserFrameView::Layout()
{
    LayoutClientView();
}

bool GlassBrowserFrameView::HitTest(const gfx::Point& l) const
{
    return !GetWindow()->client_view()->bounds().Contains(l);
}

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, private:

int GlassBrowserFrameView::FrameBorderThickness() const
{
    view::Window* window = frame_->GetWindow();
    return (window->IsMaximized() || window->IsFullscreen()) ?
        0 : GetSystemMetrics(SM_CXSIZEFRAME);
}

int GlassBrowserFrameView::NonClientBorderThickness() const
{
    view::Window* window = frame_->GetWindow();
    if(window->IsMaximized() || window->IsFullscreen())
    {
        return 0;
    }

    return kNonClientBorderThickness;
}

int GlassBrowserFrameView::NonClientTopBorderHeight(bool restored,
                                                    bool ignore_vertical_tabs) const
{
    if(!restored && frame_->GetWindow()->IsFullscreen())
    {
        return 0;
    }
    // We'd like to use FrameBorderThickness() here, but the maximized Aero glass
    // frame has a 0 frame border around most edges and a CYSIZEFRAME-thick border
    // at the top (see AeroGlassFrame::OnGetMinMaxInfo()).
    return GetSystemMetrics(SM_CYSIZEFRAME) +
        ((!restored && browser_view_->IsMaximized()) ?
        -kTabstripTopShadowThickness : kNonClientRestoredExtraThickness);
}

void GlassBrowserFrameView::PaintToolbarBackground(gfx::Canvas* canvas)
{
}

void GlassBrowserFrameView::PaintRestoredClientEdge(gfx::Canvas* canvas)
{
    ThemeProvider* tp = GetThemeProvider();
    gfx::Rect client_area_bounds = CalculateClientAreaBounds(width(), height());

    // The client edges start below the toolbar upper corner images regardless
    // of how tall the toolbar itself is.
    int client_area_top = (frame_->GetWindow()->client_view()->y() +
        /*WLW TODO: fix it. browser_view_->GetToolbarBounds().y() +*/
        tp->GetBitmapNamed(IDR_CONTENT_TOP_LEFT_CORNER)->height());
    int client_area_bottom =
        std::max(client_area_top, height() - NonClientBorderThickness());
    int client_area_height = client_area_bottom - client_area_top;

    // Draw the client edge images.
    SkBitmap* right = tp->GetBitmapNamed(IDR_CONTENT_RIGHT_SIDE);
    canvas->TileImageInt(*right, client_area_bounds.right(), client_area_top,
        right->width(), client_area_height);
    canvas->DrawBitmapInt(
        *tp->GetBitmapNamed(IDR_CONTENT_BOTTOM_RIGHT_CORNER),
        client_area_bounds.right(), client_area_bottom);
    SkBitmap* bottom = tp->GetBitmapNamed(IDR_CONTENT_BOTTOM_CENTER);
    canvas->TileImageInt(*bottom, client_area_bounds.x(),
        client_area_bottom, client_area_bounds.width(),
        bottom->height());
    SkBitmap* bottom_left =
        tp->GetBitmapNamed(IDR_CONTENT_BOTTOM_LEFT_CORNER);
    canvas->DrawBitmapInt(*bottom_left,
        client_area_bounds.x()-bottom_left->width(), client_area_bottom);
    SkBitmap* left = tp->GetBitmapNamed(IDR_CONTENT_LEFT_SIDE);
    canvas->TileImageInt(*left, client_area_bounds.x()-left->width(),
        client_area_top, left->width(), client_area_height);

    // Draw the toolbar color so that the client edges show the right color even
    // where not covered by the toolbar image.  NOTE: We do this after drawing the
    // images because the images are meant to alpha-blend atop the frame whereas
    // these rects are meant to be fully opaque, without anything overlaid.
    SkColor toolbar_color = tp->GetColor(BrowserThemeProvider::COLOR_TOOLBAR);
    canvas->FillRectInt(toolbar_color,
        client_area_bounds.x()-kClientEdgeThickness, client_area_top,
        kClientEdgeThickness,
        client_area_bottom+kClientEdgeThickness-client_area_top);
    canvas->FillRectInt(toolbar_color, client_area_bounds.x(), client_area_bottom,
        client_area_bounds.width(), kClientEdgeThickness);
    canvas->FillRectInt(toolbar_color, client_area_bounds.right(),
        client_area_top, kClientEdgeThickness,
        client_area_bottom+kClientEdgeThickness-client_area_top);
}

void GlassBrowserFrameView::LayoutClientView()
{
    client_view_bounds_ = CalculateClientAreaBounds(width(), height());
}

gfx::Rect GlassBrowserFrameView::CalculateClientAreaBounds(int width,
                                                           int height) const
{
    if(!browser_view_->IsTabStripVisible())
    {
        return gfx::Rect(0, 0, this->width(), this->height());
    }

    int top_height = NonClientTopBorderHeight(false, false);
    int border_thickness = NonClientBorderThickness();
    return gfx::Rect(border_thickness, top_height,
        std::max(0, width-(2*border_thickness)),
        std::max(0, height-top_height-border_thickness));
}