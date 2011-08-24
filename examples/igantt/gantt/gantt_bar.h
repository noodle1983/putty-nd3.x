
#ifndef __gantt_bar_h__
#define __gantt_bar_h__

#pragma once

#include "view/view.h"

class GanttBar : public view::View
{
public:
    GanttBar(const base::Time& start_time, const base::Time& end_time);
    virtual ~GanttBar();

    // Maximum value of progress.
    static const int kMaxProgress;

    void SetProgress(int progress);

    // Overriden from view::View
    virtual std::string GetClassName() const;
    virtual gfx::Size GetPreferredSize();
    virtual bool OnSetCursor(const gfx::Point& p);
    virtual bool OnMousePressed(const view::MouseEvent& event);
    virtual bool OnMouseDragged(const view::MouseEvent& event);
    virtual void OnMouseReleased(const view::MouseEvent& event);
    virtual void OnMouseCaptureLost();
    virtual void GetAccessibleState(ui::AccessibleViewState* state);

protected:
    // Overriden from view::View
    virtual void OnPaint(gfx::Canvas* canvas);

private:
    enum DragMode
    {
        DRAGMODE_NONE,
        DRAGMODE_RESIZE_LEFT,
        DRAGMODE_MOVE,
        DRAGMODE_RESIZE_RIGHT
    };

    GanttBar::DragMode GetDragMode(const gfx::Point& point) const;

    base::Time start_time_;
    base::Time end_time_;
    gfx::Rect start_drag_bounds_;
    gfx::Point start_drag_mouse_point_;
    DragMode drag_mode_;

    // Progress in range [0, kMaxProgress].
    int progress_;

    // The view class name.
    static const char kViewClassName[];

    DISALLOW_COPY_AND_ASSIGN(GanttBar);
};

#endif //__gantt_bar_h__