
#ifndef __gantt_view_h__
#define __gantt_view_h__

#pragma once

#include "base/memory/scoped_ptr.h"
#include "view/view.h"

class GanttView : public view::View
{
public:
    GanttView();
    virtual ~GanttView();

    // Overriden from view::View
    virtual bool OnMousePressed(const view::MouseEvent& event);
    virtual bool OnMouseDragged(const view::MouseEvent& event);
    virtual void OnMouseReleased(const view::MouseEvent& event);
    virtual void OnMouseCaptureLost();

private:
    view::View* select_frame_;
    gfx::Point start_select_point_;

    DISALLOW_COPY_AND_ASSIGN(GanttView);
};

#endif //__gantt_view_h__