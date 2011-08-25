
#include "border_content.h"

#include <algorithm>

#include "base/logging.h"

#include "SkPaint.h"

static const int kLeftMargin = 6;
static const int kTopMargin = 6;
static const int kRightMargin = 6;
static const int kBottomMargin = 9;

namespace
{
    MONITORINFO GetMonitorInfoForMonitor(HMONITOR monitor)
    {
        MONITORINFO monitor_info = { 0 };
        monitor_info.cbSize = sizeof(monitor_info);
        GetMonitorInfo(monitor, &monitor_info);
        return monitor_info;
    }
}

BorderContent::BorderContent() : bubble_border_(NULL),
content_margins_(kTopMargin, kLeftMargin, kBottomMargin, kRightMargin) {}

void BorderContent::Init()
{
    // Default arrow location.
    BubbleBorder::ArrowLocation arrow_location = BubbleBorder::TOP_LEFT;
    if(base::i18n::IsRTL())
    {
        arrow_location = BubbleBorder::horizontal_mirror(arrow_location);
    }
    DCHECK(!bubble_border_);

    bubble_border_ = new BubbleBorder(arrow_location);
    set_border(bubble_border_);
    set_background(new BubbleBackground(bubble_border_));
}

void BorderContent::SetBackgroundColor(SkColor color)
{
    bubble_border_->set_background_color(color);
}

void BorderContent::SizeAndGetBounds(const gfx::Rect& position_relative_to,
                                     BubbleBorder::ArrowLocation arrow_location,
                                     bool allow_bubble_offscreen,
                                     const gfx::Size& content_size,
                                     gfx::Rect* content_bounds,
                                     gfx::Rect* window_bounds)
{
    if(base::i18n::IsRTL())
    {
        arrow_location = BubbleBorder::horizontal_mirror(arrow_location);
    }
    bubble_border_->set_arrow_location(arrow_location);
    // Set the border.
    set_border(bubble_border_);

    // Give the content a margin.
    gfx::Size local_content_size(content_size);
    local_content_size.Enlarge(content_margins_.width(),
        content_margins_.height());

    // Try putting the arrow in its initial location, and calculating the bounds.
    *window_bounds =
        bubble_border_->GetBounds(position_relative_to, local_content_size);
    if(!allow_bubble_offscreen)
    {
        gfx::Rect monitor_bounds = GetMonitorBounds(position_relative_to);
        if(!monitor_bounds.IsEmpty())
        {
            // Try to resize vertically if this does not fit on the screen.
            MirrorArrowIfOffScreen(
                true, // |vertical|.
                position_relative_to, monitor_bounds,
                local_content_size, &arrow_location,
                window_bounds);
            // Then try to resize horizontally if it still does not fit on the screen.
            MirrorArrowIfOffScreen(
                false, // |vertical|.
                position_relative_to, monitor_bounds,
                local_content_size, &arrow_location,
                window_bounds);
        }
    }

    // Calculate the bounds of the contained content (in window coordinates) by
    // subtracting the border dimensions and margin amounts.
    *content_bounds = gfx::Rect(gfx::Point(), window_bounds->size());
    gfx::Insets insets;
    bubble_border_->GetInsets(&insets);
    insets += content_margins_;
    content_bounds->Inset(insets);
}

gfx::Rect BorderContent::GetMonitorBounds(const gfx::Rect& rect)
{
    RECT other_bounds_rect = rect.ToRECT();
    MONITORINFO monitor_info = GetMonitorInfoForMonitor(MonitorFromRect(
        &other_bounds_rect, MONITOR_DEFAULTTONEAREST));
    return gfx::Rect(monitor_info.rcWork);
}

void BorderContent::MirrorArrowIfOffScreen(
    bool vertical,
    const gfx::Rect& position_relative_to,
    const gfx::Rect& monitor_bounds,
    const gfx::Size& local_content_size,
    BubbleBorder::ArrowLocation* arrow_location,
    gfx::Rect* window_bounds)
{
    // If the bounds don't fit, move the arrow to its mirrored position to see if
    // it improves things.
    gfx::Insets offscreen_insets;
    if(ComputeOffScreenInsets(monitor_bounds, *window_bounds,
        &offscreen_insets) &&
        GetInsetsLength(offscreen_insets, vertical) > 0)
    {
        BubbleBorder::ArrowLocation original_arrow_location = *arrow_location;
        *arrow_location =
            vertical ? BubbleBorder::vertical_mirror(*arrow_location) :
            BubbleBorder::horizontal_mirror(*arrow_location);

        // Change the arrow and get the new bounds.
        bubble_border_->set_arrow_location(*arrow_location);
        *window_bounds = bubble_border_->GetBounds(position_relative_to,
            local_content_size);
        gfx::Insets new_offscreen_insets;
        // If there is more of the window offscreen, we'll keep the old arrow.
        if(ComputeOffScreenInsets(monitor_bounds, *window_bounds,
            &new_offscreen_insets) &&
            GetInsetsLength(new_offscreen_insets, vertical) >=
            GetInsetsLength(offscreen_insets, vertical))
        {
            *arrow_location = original_arrow_location;
            bubble_border_->set_arrow_location(*arrow_location);
            *window_bounds = bubble_border_->GetBounds(position_relative_to,
                local_content_size);
        }
    }
}

// static
bool BorderContent::ComputeOffScreenInsets(const gfx::Rect& monitor_bounds,
                                           const gfx::Rect& window_bounds,
                                           gfx::Insets* offscreen_insets)
{
    if(monitor_bounds.Contains(window_bounds))
    {
        return false;
    }

    if(!offscreen_insets)
    {
        return true;
    }

    //  window_bounds
    //  +-------------------------------+
    //  |             top               |
    //  |      +----------------+       |
    //  | left | monitor_bounds | right |
    //  |      +----------------+       |
    //  |            bottom             |
    //  +-------------------------------+
    int top = std::max(0, monitor_bounds.y() - window_bounds.y());
    int left = std::max(0, monitor_bounds.x() - window_bounds.x());
    int bottom = std::max(0, window_bounds.bottom() - monitor_bounds.bottom());
    int right = std::max(0, window_bounds.right() - monitor_bounds.right());

    offscreen_insets->Set(top, left, bottom, right);
    return true;
}

// static
int BorderContent::GetInsetsLength(const gfx::Insets& insets, bool vertical)
{
    return vertical ? insets.height() : insets.width();
}