
#include "opaque_browser_frame_view.h"

#include "gfx/canvas.h"
#include "gfx/font.h"

#include "view/accessibility/accessible_view_state.h"
#include "view/base/app_res_ids.h"
#include "view/base/resource_bundle.h"
#include "view/base/window_shape.h"
#include "view/controls/button/image_button.h"
#include "view/l10n/l10n_util.h"
#include "view/window/window.h"

#include "../../wanui_res/resource.h"

#include "browser_frame_win.h"
#include "browser_view.h"

namespace
{
    // The frame border is only visible in restored mode and is hardcoded to 4 px on
    // each side regardless of the system window border size.
    const int kFrameBorderThickness = 4;
    // Besides the frame border, there's another 11 px of empty space atop the
    // window in restored mode, to use to drag the window around.
    const int kNonClientRestoredExtraThickness = 11;
    // While resize areas on Windows are normally the same size as the window
    // borders, our top area is shrunk by 1 px to make it easier to move the window
    // around with our thinner top grabbable strip.  (Incidentally, our side and
    // bottom resize areas don't match the frame border thickness either -- they
    // span the whole nonclient area, so there's no "dead zone" for the mouse.)
    const int kTopResizeAdjust = 1;
    // In the window corners, the resize areas don't actually expand bigger, but the
    // 16 px at the end of each edge triggers diagonal resizing.
    const int kResizeAreaCornerSize = 16;
    // The titlebar never shrinks too short to show the caption button plus some
    // padding below it.
    const int kCaptionButtonHeightWithPadding = 19;
    // The content left/right images have a shadow built into them.
    const int kContentEdgeShadowThickness = 2;
    // The titlebar has a 2 px 3D edge along the top and bottom.
    const int kTitlebarTopAndBottomEdgeThickness = 2;
    // The icon is inset 2 px from the left frame border.
    const int kIconLeftSpacing = 2;
    // The icon never shrinks below 16 px on a side.
    const int kIconMinimumSize = 16;
    // There is a 4 px gap between the icon and the title text.
    const int kIconTitleSpacing = 4;
    // There is a 5 px gap between the title text and the caption buttons.
    const int kTitleLogoSpacing = 5;
    // The OTR avatar ends 2 px above the bottom of the tabstrip (which, given the
    // way the tabstrip draws its bottom edge, will appear like a 1 px gap to the
    // user).
    const int kOTRBottomSpacing = 2;
    // There are 2 px on each side of the OTR avatar (between the frame border and
    // it on the left, and between it and the tabstrip on the right).
    const int kOTRSideSpacing = 2;
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
    // How far to indent the tabstrip from the left side of the screen when there
    // is no OTR icon.
    const int kTabStripIndent = 1;
    // Inset from the top of the toolbar/tabstrip to the shadow. Used only for
    // vertical tabs.
    const int kVerticalTabBorderInset = 3;
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, public:

OpaqueBrowserFrameView::OpaqueBrowserFrameView(BrowserFrameWin* frame,
                                               BrowserView* browser_view)
                                               : BrowserNonClientFrameView(),
                                               minimize_button_(new view::ImageButton(this)),
                                               maximize_button_(new view::ImageButton(this)),
                                               restore_button_(new view::ImageButton(this)),
                                               close_button_(new view::ImageButton(this)),
                                               frame_(frame),
                                               browser_view_(browser_view)
{
    ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    SkBitmap* background =
        rb.GetBitmapNamed(IDR_THEME_WINDOW_CONTROL_BACKGROUND);
    minimize_button_->SetImage(view::CustomButton::BS_NORMAL,
        rb.GetBitmapNamed(IDR_MINIMIZE));
    minimize_button_->SetImage(view::CustomButton::BS_HOT,
        rb.GetBitmapNamed(IDR_MINIMIZE_H));
    minimize_button_->SetImage(view::CustomButton::BS_PUSHED,
        rb.GetBitmapNamed(IDR_MINIMIZE_P));
    minimize_button_->SetAccessibleName(view::GetStringUTF16(IDS_ACCNAME_MINIMIZE));
    AddChildView(minimize_button_);

    maximize_button_->SetImage(view::CustomButton::BS_NORMAL,
        rb.GetBitmapNamed(IDR_MAXIMIZE));
    maximize_button_->SetImage(view::CustomButton::BS_HOT,
        rb.GetBitmapNamed(IDR_MAXIMIZE_H));
    maximize_button_->SetImage(view::CustomButton::BS_PUSHED,
        rb.GetBitmapNamed(IDR_MAXIMIZE_P));
    maximize_button_->SetAccessibleName(view::GetStringUTF16(IDS_ACCNAME_MAXIMIZE));
    AddChildView(maximize_button_);

    restore_button_->SetImage(view::CustomButton::BS_NORMAL,
        rb.GetBitmapNamed(IDR_RESTORE));
    restore_button_->SetImage(view::CustomButton::BS_HOT,
        rb.GetBitmapNamed(IDR_RESTORE_H));
    restore_button_->SetImage(view::CustomButton::BS_PUSHED,
        rb.GetBitmapNamed(IDR_RESTORE_P));
    restore_button_->SetAccessibleName(view::GetStringUTF16(IDS_ACCNAME_RESTORE));
    AddChildView(restore_button_);

    close_button_->SetImage(view::CustomButton::BS_NORMAL,
        rb.GetBitmapNamed(IDR_CLOSE));
    close_button_->SetImage(view::CustomButton::BS_HOT,
        rb.GetBitmapNamed(IDR_CLOSE_H));
    close_button_->SetImage(view::CustomButton::BS_PUSHED,
        rb.GetBitmapNamed(IDR_CLOSE_P));
    close_button_->SetAccessibleName(view::GetStringUTF16(IDS_ACCNAME_CLOSE));
    AddChildView(close_button_);
}

OpaqueBrowserFrameView::~OpaqueBrowserFrameView() {}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, BrowserNonClientFrameView implementation:

gfx::Rect OpaqueBrowserFrameView::GetBoundsForTabStrip(view::View* tabstrip) const
{
    return gfx::Rect();
}

int OpaqueBrowserFrameView::GetHorizontalTabStripVerticalOffset(
    bool restored) const
{
    return NonClientTopBorderHeight(restored, true) + ((!restored &&
        (frame_->GetWindow()->IsMaximized() ||
        frame_->GetWindow()->IsFullscreen())) ?
        0 : kNonClientRestoredExtraThickness);
}

gfx::Size OpaqueBrowserFrameView::GetMinimumSize()
{
    gfx::Size min_size(browser_view_->GetMinimumSize());
    int border_thickness = NonClientBorderThickness();
    min_size.Enlarge(2*border_thickness,
        NonClientTopBorderHeight(false, false)+border_thickness);

    view::WindowDelegate* delegate = frame_->GetWindow()->window_delegate();
    int min_titlebar_width = (2 * FrameBorderThickness(false)) +
        kIconLeftSpacing +
        (delegate && delegate->ShouldShowWindowIcon() ?
        (IconSize() + kTitleLogoSpacing) : 0);
    min_titlebar_width +=
        minimize_button_->GetMinimumSize().width() +
        restore_button_->GetMinimumSize().width() +
        close_button_->GetMinimumSize().width();
    min_size.set_width(std::max(min_size.width(), min_titlebar_width));
    return min_size;
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, protected:

int OpaqueBrowserFrameView::GetReservedHeight() const
{
    return 0;
}

gfx::Rect OpaqueBrowserFrameView::GetBoundsForReservedArea() const
{
    gfx::Rect client_view_bounds = CalculateClientAreaBounds(width(), height());
    return gfx::Rect(client_view_bounds.x(),
        client_view_bounds.y()+client_view_bounds.height(),
        client_view_bounds.width(),
        GetReservedHeight());
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, view::NonClientFrameView implementation:

gfx::Rect OpaqueBrowserFrameView::GetBoundsForClientView() const
{
    return client_view_bounds_;
}

bool OpaqueBrowserFrameView::AlwaysUseNativeFrame() const
{
    return frame_->AlwaysUseNativeFrame();
}

bool OpaqueBrowserFrameView::AlwaysUseCustomFrame() const
{
    return true;
}

gfx::Rect OpaqueBrowserFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const
{
    int top_height = NonClientTopBorderHeight(false, false);
    int border_thickness = NonClientBorderThickness();
    return gfx::Rect(std::max(0, client_bounds.x()-border_thickness),
        std::max(0, client_bounds.y()-top_height),
        client_bounds.width()+(2*border_thickness),
        client_bounds.height()+top_height+border_thickness);
}

int OpaqueBrowserFrameView::NonClientHitTest(const gfx::Point& point)
{
    if(!bounds().Contains(point))
    {
        return HTNOWHERE;
    }

    int frame_component =
        frame_->GetWindow()->client_view()->NonClientHitTest(point);

    // See if we're in the sysmenu region.  We still have to check the tabstrip
    // first so that clicks in a tab don't get treated as sysmenu clicks.
    gfx::Rect sysmenu_rect(IconBounds());
    // In maximized mode we extend the rect to the screen corner to take advantage
    // of Fitts' Law.
    if(frame_->GetWindow()->IsMaximized())
    {
        sysmenu_rect.SetRect(0, 0, sysmenu_rect.right(), sysmenu_rect.bottom());
    }
    sysmenu_rect.set_x(GetMirroredXForRect(sysmenu_rect));
    if(sysmenu_rect.Contains(point))
    {
        return (frame_component == HTCLIENT) ? HTCLIENT : HTSYSMENU;
    }

    if(frame_component != HTNOWHERE)
    {
        return frame_component;
    }

    // Then see if the point is within any of the window controls.
    if(close_button_->IsVisible() &&
        close_button_->GetMirroredBounds().Contains(point))
    {
        return HTCLOSE;
    }
    if(restore_button_->IsVisible() &&
        restore_button_->GetMirroredBounds().Contains(point))
    {
        return HTMAXBUTTON;
    }
    if(maximize_button_->IsVisible() &&
        maximize_button_->GetMirroredBounds().Contains(point))
    {
        return HTMAXBUTTON;
    }
    if(minimize_button_->IsVisible() &&
        minimize_button_->GetMirroredBounds().Contains(point))
    {
        return HTMINBUTTON;
    }

    view::WindowDelegate* delegate = frame_->GetWindow()->window_delegate();
    if(!delegate)
    {
        LOG(WARNING) << "delegate is NULL, returning safe default.";
        return HTCAPTION;
    }
    int window_component = GetHTComponentForFrame(point, TopResizeHeight(),
        NonClientBorderThickness(), kResizeAreaCornerSize, kResizeAreaCornerSize,
        delegate->CanResize());
    // Fall back to the caption if no other component matches.
    return (window_component == HTNOWHERE) ? HTCAPTION : window_component;
}

void OpaqueBrowserFrameView::GetWindowMask(const gfx::Size& size,
                                           gfx::Path* window_mask)
{
    DCHECK(window_mask);

    if(frame_->GetWindow()->IsMaximized() || frame_->GetWindow()->IsFullscreen())
    {
        return;
    }

    view::GetDefaultWindowMask(size, window_mask);
}

void OpaqueBrowserFrameView::EnableClose(bool enable)
{
    close_button_->SetEnabled(enable);
}

void OpaqueBrowserFrameView::ResetWindowControls()
{
    restore_button_->SetState(view::CustomButton::BS_NORMAL);
    minimize_button_->SetState(view::CustomButton::BS_NORMAL);
    maximize_button_->SetState(view::CustomButton::BS_NORMAL);
    // The close button isn't affected by this constraint.
}

void OpaqueBrowserFrameView::UpdateWindowIcon()
{
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, view::View overrides:

void OpaqueBrowserFrameView::OnPaint(gfx::Canvas* canvas)
{
    view::Window* window = frame_->GetWindow();
    if(window->IsFullscreen())
    {
        return; // Nothing is visible, so don't bother to paint.
    }

    if(window->IsMaximized())
    {
        PaintMaximizedFrameBorder(canvas);
    }
    else
    {
        PaintRestoredFrameBorder(canvas);
    }
    PaintTitleBar(canvas);
    //if(browser_view_->IsToolbarVisible())
    //{
    //    PaintToolbarBackground(canvas);
    //}
    if(!window->IsMaximized())
    {
        PaintRestoredClientEdge(canvas);
    }
}

void OpaqueBrowserFrameView::Layout()
{
    LayoutWindowControls();
    LayoutTitleBar();
    client_view_bounds_ = CalculateClientAreaBounds(width(), height());
}

bool OpaqueBrowserFrameView::HitTest(const gfx::Point& l) const
{
    // If the point is outside the bounds of the client area, claim it.
    bool in_nonclient = NonClientFrameView::HitTest(l);
    if(in_nonclient)
    {
        return in_nonclient;
    }

    // Otherwise claim it only if it's in a non-tab portion of the tabstrip.
    gfx::Rect tabstrip_bounds = GetBoundsForTabStrip(NULL);
    gfx::Point tabstrip_origin(tabstrip_bounds.origin());
    View::ConvertPointToView(frame_->GetWindow()->client_view(),
        this, &tabstrip_origin);
    tabstrip_bounds.set_origin(tabstrip_origin);
    if(l.y() > tabstrip_bounds.bottom())
    {
        return false;
    }

    // We convert from our parent's coordinates since we assume we fill its bounds
    // completely. We need to do this since we're not a parent of the tabstrip,
    // meaning ConvertPointToView would otherwise return something bogus.
    gfx::Point browser_view_point(l);
    View::ConvertPointToView(parent(), browser_view_, &browser_view_point);
    //return browser_view_->IsPositionInWindowCaption(browser_view_point);
    return false;
}

void OpaqueBrowserFrameView::GetAccessibleState(AccessibleViewState* state)
{
    state->role = AccessibilityTypes::ROLE_TITLEBAR;
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, view::ButtonListener implementation:

void OpaqueBrowserFrameView::ButtonPressed(view::Button* sender,
                                           const view::Event& event)
{
    view::Window* window = frame_->GetWindow();
    if(sender == minimize_button_)
    {
        window->Minimize();
    }
    else if(sender == maximize_button_)
    {
        window->Maximize();
    }
    else if(sender == restore_button_)
    {
        window->Restore();
    }
    else if(sender == close_button_)
    {
        window->CloseWindow();
    }
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, private:

int OpaqueBrowserFrameView::FrameBorderThickness(bool restored) const
{
    view::Window* window = frame_->GetWindow();
    return (!restored && (window->IsMaximized() || window->IsFullscreen())) ?
        0 : kFrameBorderThickness;
}

int OpaqueBrowserFrameView::TopResizeHeight() const
{
    return FrameBorderThickness(false) - kTopResizeAdjust;
}

int OpaqueBrowserFrameView::NonClientBorderThickness() const
{
    // When we fill the screen, we don't show a client edge.
    view::Window* window = frame_->GetWindow();
    return FrameBorderThickness(false) +
        ((window->IsMaximized() || window->IsFullscreen()) ?
        0 : kClientEdgeThickness);
}

int OpaqueBrowserFrameView::NonClientTopBorderHeight(
    bool restored,
    bool ignore_vertical_tabs) const
{
    view::Window* window = frame_->GetWindow();
    view::WindowDelegate* delegate = window->window_delegate();
    // |delegate| may be NULL if called from callback of InputMethodChanged while
    // a window is being destroyed.
    // See more discussion at http://crosbug.com/8958
    if(delegate && delegate->ShouldShowWindowTitle())
    {
        return std::max(FrameBorderThickness(restored) + IconSize(),
            CaptionButtonY(restored) + kCaptionButtonHeightWithPadding) +
            TitlebarBottomThickness(restored);
    }

    return FrameBorderThickness(restored) -
        ((/*browser_view_->IsTabStripVisible() && */!restored &&
        window->IsMaximized()) ? kTabstripTopShadowThickness : 0);
}

int OpaqueBrowserFrameView::CaptionButtonY(bool restored) const
{
    // Maximized buttons start at window top so that even if their images aren't
    // drawn flush with the screen edge, they still obey Fitts' Law.
    return (!restored && frame_->GetWindow()->IsMaximized()) ?
        FrameBorderThickness(false) : kFrameShadowThickness;
}

int OpaqueBrowserFrameView::TitlebarBottomThickness(bool restored) const
{
    return kTitlebarTopAndBottomEdgeThickness +
        ((!restored && frame_->GetWindow()->IsMaximized()) ?
        0 : kClientEdgeThickness);
}

int OpaqueBrowserFrameView::IconSize() const
{
    // This metric scales up if either the titlebar height or the titlebar font
    // size are increased.
    return GetSystemMetrics(SM_CYSMICON);
}

gfx::Rect OpaqueBrowserFrameView::IconBounds() const
{
    int size = IconSize();
    int frame_thickness = FrameBorderThickness(false);
    int y;
    view::WindowDelegate* delegate = frame_->GetWindow()->window_delegate();
    if(delegate && (delegate->ShouldShowWindowIcon() ||
        delegate->ShouldShowWindowTitle()))
    {
        // Our frame border has a different "3D look" than Windows'.  Theirs has a
        // more complex gradient on the top that they push their icon/title below;
        // then the maximized window cuts this off and the icon/title are centered
        // in the remaining space.  Because the apparent shape of our border is
        // simpler, using the same positioning makes things look slightly uncentered
        // with restored windows, so when the window is restored, instead of
        // calculating the remaining space from below the frame border, we calculate
        // from below the 3D edge.
        int unavailable_px_at_top = frame_->GetWindow()->IsMaximized() ?
            frame_thickness : kTitlebarTopAndBottomEdgeThickness;
        // When the icon is shorter than the minimum space we reserve for the
        // caption button, we vertically center it.  We want to bias rounding to put
        // extra space above the icon, since the 3D edge (+ client edge, for
        // restored windows) below looks (to the eye) more like additional space
        // than does the 3D edge (or nothing at all, for maximized windows) above;
        // hence the +1.
        y = unavailable_px_at_top + (NonClientTopBorderHeight(false, false) -
            unavailable_px_at_top - size - TitlebarBottomThickness(false) + 1) / 2;
    }
    else
    {
        // For "browser mode" windows, we use the native positioning, which is just
        // below the top frame border.
        y = frame_thickness;
    }
    return gfx::Rect(frame_thickness+kIconLeftSpacing, y, size, size);
}

void OpaqueBrowserFrameView::PaintRestoredFrameBorder(gfx::Canvas* canvas)
{
    ResourceBundle& rb = ResourceBundle::GetSharedInstance();

    SkBitmap* top_left_corner = rb.GetBitmapNamed(IDR_WINDOW_TOP_LEFT_CORNER);
    SkBitmap* top_right_corner =
        rb.GetBitmapNamed(IDR_WINDOW_TOP_RIGHT_CORNER);
    SkBitmap* top_edge = rb.GetBitmapNamed(IDR_WINDOW_TOP_CENTER);
    SkBitmap* right_edge = rb.GetBitmapNamed(IDR_WINDOW_RIGHT_SIDE);
    SkBitmap* left_edge = rb.GetBitmapNamed(IDR_WINDOW_LEFT_SIDE);
    SkBitmap* bottom_left_corner =
        rb.GetBitmapNamed(IDR_WINDOW_BOTTOM_LEFT_CORNER);
    SkBitmap* bottom_right_corner =
        rb.GetBitmapNamed(IDR_WINDOW_BOTTOM_RIGHT_CORNER);
    SkBitmap* bottom_edge = rb.GetBitmapNamed(IDR_WINDOW_BOTTOM_CENTER);

    // Window frame mode and color.
    SkBitmap* theme_frame;
    SkColor frame_color;
    {
        if(ShouldPaintAsActive())
        {
            theme_frame = rb.GetBitmapNamed(IDR_THEME_FRAME);
            //frame_color = rb.GetColor(ThemeService::COLOR_FRAME);
        }
        else
        {
            theme_frame = rb.GetBitmapNamed(IDR_THEME_FRAME_INACTIVE);
            //frame_color = rb.GetColor(ThemeService::COLOR_FRAME_INACTIVE);
        }
    }

    // Fill with the frame color first so we have a constant background for
    // areas not covered by the theme image.
    canvas->FillRectInt(frame_color, 0, 0, width(), theme_frame->height());
    // Now fill down the sides.
    canvas->FillRectInt(frame_color, 0, theme_frame->height(), left_edge->width(),
        height()-theme_frame->height());
    canvas->FillRectInt(frame_color, width()-right_edge->width(),
        theme_frame->height(), right_edge->width(),
        height()-theme_frame->height());
    // Now fill the bottom area.
    canvas->FillRectInt(frame_color, left_edge->width(),
        height()-bottom_edge->height(),
        width()-left_edge->width()-right_edge->width(),
        bottom_edge->height());

    // Draw the theme frame.
    canvas->TileImageInt(*theme_frame, 0, 0, width(), theme_frame->height());

    // Top.
    int top_left_height = std::min(top_left_corner->height(),
        height()-bottom_left_corner->height());
    canvas->DrawBitmapInt(*top_left_corner, 0, 0, top_left_corner->width(),
        top_left_height, 0, 0, top_left_corner->width(), top_left_height, false);
    canvas->TileImageInt(*top_edge, top_left_corner->width(), 0,
        width()-top_right_corner->width(), top_edge->height());
    int top_right_height = std::min(top_right_corner->height(),
        height()-bottom_right_corner->height());
    canvas->DrawBitmapInt(*top_right_corner, 0, 0, top_right_corner->width(),
        top_right_height, width()-top_right_corner->width(), 0,
        top_right_corner->width(), top_right_height, false);
    // Note: When we don't have a toolbar, we need to draw some kind of bottom
    // edge here.  Because the App Window graphics we use for this have an
    // attached client edge and their sizing algorithm is a little involved, we do
    // all this in PaintRestoredClientEdge().

    // Right.
    canvas->TileImageInt(*right_edge, width()-right_edge->width(),
        top_right_height, right_edge->width(),
        height()-top_right_height-bottom_right_corner->height());

    // Bottom.
    canvas->DrawBitmapInt(*bottom_right_corner,
        width()-bottom_right_corner->width(),
        height()-bottom_right_corner->height());
    canvas->TileImageInt(*bottom_edge, bottom_left_corner->width(),
        height()-bottom_edge->height(),
        width()-bottom_left_corner->width()-bottom_right_corner->width(),
        bottom_edge->height());
    canvas->DrawBitmapInt(*bottom_left_corner, 0,
        height()-bottom_left_corner->height());

    // Left.
    canvas->TileImageInt(*left_edge, 0, top_left_height, left_edge->width(),
        height()-top_left_height-bottom_left_corner->height());
}

void OpaqueBrowserFrameView::PaintMaximizedFrameBorder(gfx::Canvas* canvas)
{
    ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    view::Window* window = frame_->GetWindow();

    // Window frame mode and color
    SkBitmap* theme_frame = rb.GetBitmapNamed(ShouldPaintAsActive() ?
        IDR_THEME_FRAME : IDR_THEME_FRAME_INACTIVE);
    // Draw the theme frame.  It must be aligned with the tabstrip as if we were
    // in restored mode.  Note that the top of the tabstrip is
    // kTabstripTopShadowThickness px off the top of the screen.
    int theme_background_y = -(GetHorizontalTabStripVerticalOffset(true) +
        kTabstripTopShadowThickness);
    canvas->TileImageInt(*theme_frame, 0, theme_background_y, width(),
        theme_frame->height());

    //if(!browser_view_->IsToolbarVisible())
    //{
    //    // There's no toolbar to edge the frame border, so we need to draw a bottom
    //    // edge.  The graphic we use for this has a built in client edge, so we clip
    //    // it off the bottom.
    //    SkBitmap* top_center =
    //        rb.GetBitmapNamed(IDR_APP_TOP_CENTER);
    //    int edge_height = top_center->height() - kClientEdgeThickness;
    //    canvas->TileImageInt(*top_center, 0,
    //        window->client_view()->y()-edge_height, width(), edge_height);
    //}
}

void OpaqueBrowserFrameView::PaintTitleBar(gfx::Canvas* canvas)
{
    // The window icon is painted by the TabIconView.
    view::WindowDelegate* delegate = frame_->GetWindow()->window_delegate();
    if(!delegate)
    {
        LOG(WARNING) << "delegate is NULL";
        return;
    }
    if(delegate->ShouldShowWindowTitle())
    {
        canvas->DrawStringInt(delegate->GetWindowTitle(),
            BrowserFrameWin::GetTitleFont(),
            SK_ColorWHITE, GetMirroredXForRect(title_bounds_),
            title_bounds_.y(), title_bounds_.width(), title_bounds_.height());
        /* TODO(pkasting):  If this window is active, we should also draw a drop
        * shadow on the title.  This is tricky, because we don't want to hardcode a
        * shadow color (since we want to work with various themes), but we can't
        * alpha-blend either (since the Windows text APIs don't really do this).
        * So we'd need to sample the background color at the right location and
        * synthesize a good shadow color. */
    }
}

void OpaqueBrowserFrameView::PaintToolbarBackground(gfx::Canvas* canvas)
{
}

void OpaqueBrowserFrameView::PaintRestoredClientEdge(gfx::Canvas* canvas)
{
    ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    int client_area_top = frame_->GetWindow()->client_view()->y();
    int image_top = client_area_top;

    gfx::Rect client_area_bounds = CalculateClientAreaBounds(width(), height());
    SkColor toolbar_color/* = rb.GetColor(ThemeService::COLOR_TOOLBAR)*/;

    // The client edge images always start below the toolbar corner images.  The
    // client edge filled rects start there or at the bottom of the tooolbar,
    // whichever is shorter.
    gfx::Rect toolbar_bounds/*(browser_view_->GetToolbarBounds())*/;
    image_top += toolbar_bounds.y() +
        rb.GetBitmapNamed(IDR_CONTENT_TOP_LEFT_CORNER)->height();
    client_area_top = std::min(image_top,
        client_area_top+toolbar_bounds.bottom()-kClientEdgeThickness);

    int client_area_bottom =
        std::max(client_area_top, height()-NonClientBorderThickness());
    int image_height = client_area_bottom - image_top;

    // Draw the client edge images.
    SkBitmap* right = rb.GetBitmapNamed(IDR_CONTENT_RIGHT_SIDE);
    canvas->TileImageInt(*right, client_area_bounds.right(), image_top,
        right->width(), image_height);
    canvas->DrawBitmapInt(
        *rb.GetBitmapNamed(IDR_CONTENT_BOTTOM_RIGHT_CORNER),
        client_area_bounds.right(), client_area_bottom);
    SkBitmap* bottom = rb.GetBitmapNamed(IDR_CONTENT_BOTTOM_CENTER);
    canvas->TileImageInt(*bottom, client_area_bounds.x(),
        client_area_bottom, client_area_bounds.width(),
        bottom->height());
    SkBitmap* bottom_left =
        rb.GetBitmapNamed(IDR_CONTENT_BOTTOM_LEFT_CORNER);
    canvas->DrawBitmapInt(*bottom_left,
        client_area_bounds.x()-bottom_left->width(), client_area_bottom);
    SkBitmap* left = rb.GetBitmapNamed(IDR_CONTENT_LEFT_SIDE);
    canvas->TileImageInt(*left, client_area_bounds.x()-left->width(),
        image_top, left->width(), image_height);

    // Draw the toolbar color so that the client edges show the right color even
    // where not covered by the toolbar image.  NOTE: We do this after drawing the
    // images because the images are meant to alpha-blend atop the frame whereas
    // these rects are meant to be fully opaque, without anything overlaid.
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

void OpaqueBrowserFrameView::LayoutWindowControls()
{
    bool is_maximized = frame_->GetWindow()->IsMaximized();
    close_button_->SetImageAlignment(view::ImageButton::ALIGN_LEFT,
        view::ImageButton::ALIGN_BOTTOM);
    int caption_y = CaptionButtonY(false);
    // There should always be the same number of non-shadow pixels visible to the
    // side of the caption buttons.  In maximized mode we extend the rightmost
    // button to the screen corner to obey Fitts' Law.
    int right_extra_width = is_maximized ?
        (kFrameBorderThickness - kFrameShadowThickness) : 0;
    gfx::Size close_button_size = close_button_->GetPreferredSize();
    close_button_->SetBounds(width()-FrameBorderThickness(false)-
        right_extra_width-close_button_size.width(), caption_y,
        close_button_size.width()+right_extra_width,
        close_button_size.height());

    // When the window is restored, we show a maximized button; otherwise, we show
    // a restore button.
    bool is_restored = !is_maximized && !frame_->GetWindow()->IsMinimized();
    view::ImageButton* invisible_button = is_restored ?
        restore_button_ : maximize_button_;
    invisible_button->SetVisible(false);

    view::ImageButton* visible_button = is_restored ?
        maximize_button_ : restore_button_;
    visible_button->SetVisible(true);
    visible_button->SetImageAlignment(view::ImageButton::ALIGN_LEFT,
        view::ImageButton::ALIGN_BOTTOM);
    gfx::Size visible_button_size = visible_button->GetPreferredSize();
    visible_button->SetBounds(close_button_->x()-visible_button_size.width(),
        caption_y, visible_button_size.width(),
        visible_button_size.height());

    minimize_button_->SetVisible(true);
    minimize_button_->SetImageAlignment(view::ImageButton::ALIGN_LEFT,
        view::ImageButton::ALIGN_BOTTOM);
    gfx::Size minimize_button_size = minimize_button_->GetPreferredSize();
    minimize_button_->SetBounds(
        visible_button->x()-minimize_button_size.width(), caption_y,
        minimize_button_size.width(),
        minimize_button_size.height());
}

void OpaqueBrowserFrameView::LayoutTitleBar()
{
    // The window title is based on the calculated icon position, even when there
    // is no icon.
    gfx::Rect icon_bounds(IconBounds());
    view::WindowDelegate* delegate = frame_->GetWindow()->window_delegate();
    if(delegate && delegate->ShouldShowWindowIcon())
    {
    }

    // Size the title, if visible.
    if(delegate && delegate->ShouldShowWindowTitle())
    {
        int title_x = delegate->ShouldShowWindowIcon() ?
            icon_bounds.right()+kIconTitleSpacing : icon_bounds.x();
        int title_height = BrowserFrameWin::GetTitleFont().GetHeight();
        // We bias the title position so that when the difference between the icon
        // and title heights is odd, the extra pixel of the title is above the
        // vertical midline rather than below.  This compensates for how the icon is
        // already biased downwards (see IconBounds()) and helps prevent descenders
        // on the title from overlapping the 3D edge at the bottom of the titlebar.
        title_bounds_.SetRect(title_x,
            icon_bounds.y()+((icon_bounds.height()-title_height-1)/2),
            std::max(0, minimize_button_->x()-kTitleLogoSpacing-title_x),
            title_height);
    }
}

gfx::Rect OpaqueBrowserFrameView::CalculateClientAreaBounds(int width,
                                                            int height) const
{
    int top_height = NonClientTopBorderHeight(false, false);
    int border_thickness = NonClientBorderThickness();
    return gfx::Rect(border_thickness, top_height,
        std::max(0, width-(2*border_thickness)),
        std::max(0, height-GetReservedHeight()-top_height-border_thickness));
}