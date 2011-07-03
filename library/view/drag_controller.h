
#ifndef __view_drag_controller_h__
#define __view_drag_controller_h__

#pragma once

namespace gfx
{
    class Point;
}

namespace ui
{
    class OSExchangeData;
}

namespace view
{

    class View;

    // DragController is responsible for writing drag data for a view, as well as
    // supplying the supported drag operations. Use DragController if you don't
    // want to subclass.
    class DragController
    {
    public:
        // Writes the data for the drag.
        virtual void WriteDragDataForView(View* sender,
            const gfx::Point& press_pt,
            ui::OSExchangeData* data) = 0;

        // Returns the supported drag operations (see DragDropTypes for possible
        // values). A drag is only started if this returns a non-zero value.
        virtual int GetDragOperationsForView(View* sender,
            const gfx::Point& p) = 0;

        // Returns true if a drag operation can be started.
        // |press_pt| represents the coordinates where the mouse was initially
        // pressed down. |p| is the current mouse coordinates.
        virtual bool CanStartDragForView(View* sender,
            const gfx::Point& press_pt,
            const gfx::Point& p) = 0;

    protected:
        virtual ~DragController() {}
    };

} //namespace view

#endif //__view_drag_controller_h__