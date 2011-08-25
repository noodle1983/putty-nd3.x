
#ifndef __border_content_h__
#define __border_content_h__

#pragma once

#include "SkColor.h"

#include "view/view.h"

#include "bubble_border.h"

// This is used to paint the border of the Bubble. Windows uses this via
// BorderWidgetWin, while others can use it directly in the bubble.
class BorderContent : public view::View
{
public:
    BorderContent();

    // Must be called before this object can be used.
    void Init();

    // Sets the background color.
    void SetBackgroundColor(SkColor color);

    // Given the size of the content and the rect to point at, returns the bounds
    // of both the border and the content inside the bubble.
    // |arrow_location| specifies the preferred location for the arrow
    // anchor. If the bubble does not fit on the monitor and
    // |allow_bubble_offscreen| is false, the arrow location may change so the
    // bubble shows entirely.
    virtual void SizeAndGetBounds(
        const gfx::Rect& position_relative_to, // In screen coordinates
        BubbleBorder::ArrowLocation arrow_location,
        bool allow_bubble_offscreen,
        const gfx::Size& content_size,
        gfx::Rect* content_bounds, // Returned in window coordinates
        gfx::Rect* window_bounds); // Returned in screen coordinates

    // Sets content margins.
    void set_content_margins(const gfx::Insets& margins)
    {
        content_margins_ = margins;
    }

protected:
    virtual ~BorderContent() {}

    // Returns the bounds for the monitor showing the specified |rect|.
    // Overridden in unit-tests.
    virtual gfx::Rect GetMonitorBounds(const gfx::Rect& rect);

    // Accessor for |content_margins_|.
    const gfx::Insets& content_margins() const
    {
        return content_margins_;
    }

    BubbleBorder* bubble_border_;

private:
    // Changes |arrow_location| to its mirrored version, vertically if |vertical|
    // is true, horizontally otherwise, if |window_bounds| don't fit in
    // |monitor_bounds|.
    void MirrorArrowIfOffScreen(bool vertical,
        const gfx::Rect& position_relative_to,
        const gfx::Rect& monitor_bounds,
        const gfx::Size& local_content_size,
        BubbleBorder::ArrowLocation* arrow_location,
        gfx::Rect* window_bounds);

    // Computes how much |window_bounds| is off-screen of the monitor bounds
    // |monitor_bounds| and puts the values in |offscreen_insets|.
    // Returns false if |window_bounds| is actually contained in |monitor_bounds|,
    // in which case |offscreen_insets| is not modified.
    static bool ComputeOffScreenInsets(const gfx::Rect& monitor_bounds,
        const gfx::Rect& window_bounds,
        gfx::Insets* offscreen_insets);

    // Convenience method that returns the height of |insets| if |vertical| is
    // true, its width otherwise.
    static int GetInsetsLength(const gfx::Insets& insets, bool vertical);

    // Margins between the content and the inside of the border, in pixels.
    gfx::Insets content_margins_;

    DISALLOW_COPY_AND_ASSIGN(BorderContent);
};

#endif //__border_content_h__