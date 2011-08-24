
#include "gantt_bar.h"

#include "SkGradientShader.h"
#include "SkBlurMaskFilter.h"
#include "ui_gfx/canvas_skia.h"
#include "ui_base/accessibility/accessible_view_state.h"
#include "ui_base/win/screen.h"
#include "view/widget/widget.h"
#include "../cursors.h"
#include "gantt_bar_menu_controller.h"

namespace
{

    static const int kResizeThreshold = 4;

    // Corner radius for the progress bar's border.
    const int kCornerRadius = 3;

    // Progress bar's border width
    const int kBorderWidth = 1;

    static void AddRoundRectPathWithPadding(int x, int y,
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

    static void AddRoundRectPath(int x, int y,
        int w, int h,
        int corner_radius,
        SkPath* path)
    {
        static const SkScalar half = SkIntToScalar(1) / 2;
        AddRoundRectPathWithPadding(x, y, w, h, corner_radius, half, path);
    }

    static void FillRoundRect(gfx::Canvas* canvas,
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

    static void FillRoundRect(gfx::Canvas* canvas,
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

    static void StrokeRoundRect(gfx::Canvas* canvas,
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

// static
const char GanttBar::kViewClassName[] = "GanttBar";
// static: GanttBar's maximum value.
const int GanttBar::kMaxProgress = 100;

GanttBar::GanttBar(const base::Time& start_time, const base::Time& end_time)
: start_time_(start_time), end_time_(end_time), drag_mode_(DRAGMODE_NONE),
progress_(0)
{
    set_context_menu_controller(GanttBarMenuController::GetInstance());
}

GanttBar::~GanttBar()
{
}

void GanttBar::SetProgress(int progress)
{
    progress_ = progress;
    if(progress_ < 0)
    {
        progress_ = 0;
    }
    else if(progress_ > kMaxProgress)
    {
        progress_ = kMaxProgress;
    }
    SchedulePaint();
}

std::string GanttBar::GetClassName() const
{
    return kViewClassName;
}

gfx::Size GanttBar::GetPreferredSize()
{
    return gfx::Size(100, 14);
}

bool GanttBar::OnSetCursor(const gfx::Point& p)
{
    HCURSOR cursor = NULL;
    switch(GetDragMode(p))
    {
    case DRAGMODE_RESIZE_LEFT:
        cursor = cursors[CURSORSHAPE_RESIZE_LEFT];
        break;
    case DRAGMODE_MOVE:
        cursor = cursors[CURSORSHAPE_MOVE];
        break;
    case DRAGMODE_RESIZE_RIGHT:
        cursor = cursors[CURSORSHAPE_RESIZE_RIGHT];
        break;
    }

    if(cursor)
    {
        GetWidget()->SetCursor(cursor);
        return true;
    }

    return false;
}

bool GanttBar::OnMousePressed(const view::MouseEvent& event)
{
    drag_mode_ = GetDragMode(event.location());
    if(drag_mode_ == DRAGMODE_NONE)
    {
        return false;
    }

    start_drag_bounds_ = bounds();
    start_drag_mouse_point_ = ui::Screen::GetCursorScreenPoint();
    return true;
}

bool GanttBar::OnMouseDragged(const view::MouseEvent& event)
{
    DCHECK_NE(drag_mode_, DRAGMODE_NONE);

    gfx::Rect new_bounds = start_drag_bounds_;

    switch(drag_mode_)
    {
    case DRAGMODE_RESIZE_LEFT:
        {
            int delta_x = ui::Screen::GetCursorScreenPoint().x() -
                start_drag_mouse_point_.x();
            if(delta_x > start_drag_bounds_.width())
            {
                delta_x = start_drag_bounds_.width();
            }
            new_bounds.Inset(delta_x, 0, 0, 0);
        }
        break;
    case DRAGMODE_MOVE:
        {
            gfx::Point delta = ui::Screen::GetCursorScreenPoint().Subtract(
                start_drag_mouse_point_);
            new_bounds.Offset(delta);
        }
        break;
    case DRAGMODE_RESIZE_RIGHT:
        {
            int delta_x = ui::Screen::GetCursorScreenPoint().x() -
                start_drag_mouse_point_.x();
            if(delta_x < -start_drag_bounds_.width())
            {
                delta_x = -start_drag_bounds_.width();
            }
            new_bounds.Inset(0, 0, -delta_x, 0);
        }
        break;
    }

    if(new_bounds.width() <= 2)
    {
        new_bounds.set_width(2);
    }
    SetBoundsRect(new_bounds);
    return true;
}

void GanttBar::OnMouseReleased(const view::MouseEvent& event)
{
    drag_mode_ = DRAGMODE_NONE;
}

void GanttBar::OnMouseCaptureLost()
{
    drag_mode_ = DRAGMODE_NONE;
}

void GanttBar::GetAccessibleState(ui::AccessibleViewState* state)
{
    state->role = ui::AccessibilityTypes::ROLE_GRAPHIC;
}

void GanttBar::OnPaint(gfx::Canvas* canvas)
{
    View::OnPaint(canvas);

    SkColor bar_color_start = SkColorSetRGB(81, 138, 223);
    SkColor bar_color_end = SkColorSetRGB(51, 103, 205);
    SkColor background_color_start = SkColorSetRGB(212, 212, 212);
    SkColor background_color_end = SkColorSetRGB(252, 252, 252);
    SkColor border_color = SkColorSetRGB(144, 144, 144);

    FillRoundRect(canvas,
        0, 0, width(), height(),
        kCornerRadius,
        background_color_start,
        background_color_end,
        false);
    if(progress_*width() > 1)
    {
        FillRoundRect(canvas,
            0, 0,
            progress_ * width() / kMaxProgress, height(),
            kCornerRadius,
            bar_color_start,
            bar_color_end,
            false);
    }
    StrokeRoundRect(canvas,
        0, 0,
        width(), height(),
        kCornerRadius,
        border_color,
        kBorderWidth);
}

GanttBar::DragMode GanttBar::GetDragMode(const gfx::Point& point) const
{
    DragMode drag_mode = DRAGMODE_MOVE;

    int delta_right = width() - point.x();
    if(delta_right <= point.x())
    {
        if(delta_right <= kResizeThreshold)
        {
            drag_mode = DRAGMODE_RESIZE_RIGHT;
        }
    }
    else
    {
        if(point.x() <= kResizeThreshold)
        {
            drag_mode = DRAGMODE_RESIZE_LEFT;
        }
    }

    return drag_mode;
}