
#ifndef __view_progress_bar_h__
#define __view_progress_bar_h__

#pragma once

#include <string>

#include "view/view.h"

namespace gfx
{
    class Canvas;
    class Point;
    class Size;
}

namespace view
{

    /////////////////////////////////////////////////////////////////////////////
    //
    // ProgressBar class
    //
    // A progress bar is a control that indicates progress visually.
    //
    /////////////////////////////////////////////////////////////////////////////

    class ProgressBar : public View
    {
    public:
        // The value range defaults to [0.0, 1.0].
        ProgressBar();
        virtual ~ProgressBar();

        double current_value() const { return current_value_; }

        // View implementation.
        virtual gfx::Size GetPreferredSize();
        virtual std::string GetClassName() const;
        virtual void GetAccessibleState(ui::AccessibleViewState* state);
        virtual void OnPaint(gfx::Canvas* canvas);
        virtual bool GetTooltipText(const gfx::Point& p, std::wstring* tooltip);

        // Sets the inclusive range of values to be displayed.  Values outside of the
        // range will be capped when displayed.
        virtual void SetDisplayRange(double min_display_value,
            double max_display_value);

        // Sets the current value.  Values outside of the range [min_display_value_,
        // max_display_value_] will be stored unmodified and capped for display.
        virtual void SetValue(double value);

        // Sets the tooltip text.  Default behavior for a progress bar is to show no
        // tooltip on mouse hover. Calling this lets you set a custom tooltip.  To
        // revert to default behavior, call this with an empty string.
        virtual void SetTooltipText(const std::wstring& tooltip_text);

    private:
        static const char kViewClassName[];

        // Inclusive range used when displaying values.
        double min_display_value_;
        double max_display_value_;

        // Current value.  May be outside of [min_display_value_, max_display_value_].
        double current_value_;

        // Tooltip text.
        string16 tooltip_text_;

        DISALLOW_COPY_AND_ASSIGN(ProgressBar);
    };

} //namespace view

#endif //__view_progress_bar_h__