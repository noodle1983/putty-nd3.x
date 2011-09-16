
#include "progress_bar.h"

#include <algorithm>
#include <string>

#include "base/logging.h"
#include "base/string_util.h"

#include "SkGradientShader.h"
#include "SkBlurMaskFilter.h"

#include "ui_gfx/brush.h"
#include "ui_gfx/canvas_skia.h"
#include "ui_gfx/color_utils.h"
#include "ui_gfx/font.h"
#include "ui_gfx/insets.h"

#include "ui_base/accessibility/accessible_view_state.h"

#include "view/background.h"
#include "view/border.h"
#include "view/painter.h"

namespace
{

    // Corner radius for the progress bar's border.
    const int kCornerRadius = 3;

    // Progress bar's border width
    const int kBorderWidth = 1;

    void AddRoundRectPathWithPadding(int x, int y,
        int w, int h,
        int corner_radius,
        SkScalar padding,
        SkPath* path)
    {
        DCHECK(path);
        if(path == NULL)
        {
            return;
        }
        SkRect rect;
        rect.set(SkIntToScalar(x)+padding, SkIntToScalar(y)+padding,
            SkIntToScalar(x+w)-padding, SkIntToScalar(y+h)-padding);
        path->addRoundRect(rect,
            SkIntToScalar(corner_radius)-padding,
            SkIntToScalar(corner_radius)-padding);
    }

    void AddRoundRectPath(int x, int y,
        int w, int h,
        int corner_radius,
        SkPath* path)
    {
        static const SkScalar half = SkIntToScalar(1) / 2;
        AddRoundRectPathWithPadding(x, y, w, h, corner_radius, half, path);
    }

    void FillRoundRect(gfx::Canvas* canvas,
        int x, int y,
        int w, int h,
        int corner_radius,
        const SkColor colors[],
        const SkScalar points[],
        int count,
        bool gradient_horizontal)
    {
        SkPath path;
        AddRoundRectPath(x, y, w, h, corner_radius, &path);
        SkPaint paint;
        paint.setStyle(SkPaint::kFill_Style);
        paint.setFlags(SkPaint::kAntiAlias_Flag);

        SkPoint p[2];
        p[0].set(SkIntToScalar(x), SkIntToScalar(y));
        if(gradient_horizontal)
        {
            p[1].set(SkIntToScalar(x + w), SkIntToScalar(y));
        }
        else
        {
            p[1].set(SkIntToScalar(x), SkIntToScalar(y + h));
        }
        SkShader* s = SkGradientShader::CreateLinear(
            p, colors, points, count, SkShader::kClamp_TileMode, NULL);
        paint.setShader(s);
        // Need to unref shader, otherwise never deleted.
        s->unref();

        canvas->AsCanvasSkia()->drawPath(path, paint);
    }

    void FillRoundRect(gfx::Canvas* canvas,
        int x, int y,
        int w, int h,
        int corner_radius,
        SkColor gradient_start_color,
        SkColor gradient_end_color,
        bool gradient_horizontal)
    {
        if(gradient_start_color != gradient_end_color)
        {
            SkColor colors[2] = { gradient_start_color, gradient_end_color };
            FillRoundRect(canvas, x, y, w, h, corner_radius,
                colors, NULL, 2, gradient_horizontal);
        }
        else
        {
            SkPath path;
            AddRoundRectPath(x, y, w, h, corner_radius, &path);
            SkPaint paint;
            paint.setStyle(SkPaint::kFill_Style);
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setColor(gradient_start_color);
            canvas->AsCanvasSkia()->drawPath(path, paint);
        }
    }

    void StrokeRoundRect(gfx::Canvas* canvas,
        int x, int y,
        int w, int h,
        int corner_radius,
        SkColor stroke_color,
        int stroke_width)
    {
        SkPath path;
        AddRoundRectPath(x, y, w, h, corner_radius, &path);
        SkPaint paint;
        paint.setShader(NULL);
        paint.setColor(stroke_color);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setFlags(SkPaint::kAntiAlias_Flag);
        paint.setStrokeWidth(SkIntToScalar(stroke_width));
        canvas->AsCanvasSkia()->drawPath(path, paint);
    }

}

namespace view
{

    // static
    const char ProgressBar::kViewClassName[] = "view/ProgressBar";

    ProgressBar::ProgressBar() : min_display_value_(0.0),
        max_display_value_(1.0), current_value_(0.0) {}

    ProgressBar::~ProgressBar() {}

    gfx::Size ProgressBar::GetPreferredSize()
    {
        return gfx::Size(100, 16);
    }

    std::string ProgressBar::GetClassName() const
    {
        return kViewClassName;
    }

    void ProgressBar::GetAccessibleState(ui::AccessibleViewState* state)
    {
        state->role = ui::AccessibilityTypes::ROLE_PROGRESSBAR;
        state->state = ui::AccessibilityTypes::STATE_READONLY;
    }

    void ProgressBar::OnPaint(gfx::Canvas* canvas)
    {
        const double capped_value = std::min(
            std::max(current_value_, min_display_value_),
            max_display_value_);
        const double capped_fraction = (capped_value - min_display_value_) /
            (max_display_value_ - min_display_value_);
        const int progress_width = static_cast<int>(width() * capped_fraction + 0.5);

        SkColor bar_color_start = SkColorSetRGB(81, 138, 223);
        SkColor bar_color_end = SkColorSetRGB(51, 103, 205);
        SkColor background_color_start = SkColorSetRGB(212, 212, 212);
        SkColor background_color_end = SkColorSetRGB(252, 252, 252);
        SkColor border_color = SkColorSetRGB(144, 144, 144);

        FillRoundRect(canvas, 0, 0,
            width(), height(),
            kCornerRadius,
            background_color_start,
            background_color_end,
            false);
        if(progress_width > 1)
        {
            FillRoundRect(canvas, 0, 0,
                progress_width, height(),
                kCornerRadius,
                bar_color_start,
                bar_color_end,
                false);
        }
        StrokeRoundRect(canvas, 0, 0,
            width(), height(),
            kCornerRadius,
            border_color,
            kBorderWidth);
    }

    bool ProgressBar::GetTooltipText(const gfx::Point& p, string16* tooltip)
    {
        DCHECK(tooltip);
        if(tooltip == NULL)
        {
            return false;
        }
        tooltip->assign(tooltip_text_);
        return !tooltip_text_.empty();
    }

    void ProgressBar::SetDisplayRange(double min_display_value,
        double max_display_value)
    {
        if(min_display_value!=min_display_value_ ||
            max_display_value!=max_display_value_)
        {
            DCHECK_LT(min_display_value, max_display_value);
            min_display_value_ = min_display_value;
            max_display_value_ = max_display_value;
            SchedulePaint();
        }
    }

    void ProgressBar::SetValue(double value)
    {
        if(value != current_value_)
        {
            current_value_ = value;
            SchedulePaint();
        }
    }

    void ProgressBar::SetTooltipText(const string16& tooltip_text)
    {
        tooltip_text_ = tooltip_text;
    }

} //namespace view