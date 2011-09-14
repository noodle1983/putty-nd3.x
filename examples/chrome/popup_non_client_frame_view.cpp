
#include "popup_non_client_frame_view.h"

#include "ui_gfx/point.h"
#include "ui_gfx/rect.h"
#include "ui_gfx/size.h"

#include "browser_frame.h"

PopupNonClientFrameView::PopupNonClientFrameView(BrowserFrame* frame)
{
    frame->set_frame_type(view::Widget::FRAME_TYPE_FORCE_NATIVE);
}

gfx::Rect PopupNonClientFrameView::GetBoundsForClientView() const
{
    return gfx::Rect(0, 0, width(), height());
}

gfx::Rect PopupNonClientFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const
{
    return client_bounds;
}

int PopupNonClientFrameView::NonClientHitTest(const gfx::Point& point)
{
    return bounds().Contains(point) ? HTCLIENT : HTNOWHERE;
}

void PopupNonClientFrameView::GetWindowMask(const gfx::Size& size,
                                            gfx::Path* window_mask) {}

void PopupNonClientFrameView::EnableClose(bool enable) {}

void PopupNonClientFrameView::ResetWindowControls() {}

void PopupNonClientFrameView::UpdateWindowIcon() {}

gfx::Rect PopupNonClientFrameView::GetBoundsForTabStrip(
    view::View* tabstrip) const
{
    return gfx::Rect(0, 0, width(), tabstrip->GetPreferredSize().height());
}

int PopupNonClientFrameView::GetHorizontalTabStripVerticalOffset(
    bool restored) const
{
    return 0;
}

void PopupNonClientFrameView::UpdateThrobber(bool running) {}