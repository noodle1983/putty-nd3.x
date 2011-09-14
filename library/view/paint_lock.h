
#ifndef __view_paint_lock_h__
#define __view_paint_lock_h__

#pragma once

#include "base/basic_types.h"

namespace view
{

    class View;

    // Instances of PaintLock can be created to disable painting of the view
    // (compositing is not disabled). When the class is destroyed, painting is
    // re-enabled. This can be useful during operations like animations, that are
    // sensitive to costly paints, and during which only composting, not painting,
    // is required.
    class PaintLock
    {
    public:
        // The paint lock does not own the view. It is an error for the view to be
        // destroyed before the lock.
        PaintLock(View* view);
        ~PaintLock();

    private:
        View* view_;

        DISALLOW_COPY_AND_ASSIGN(PaintLock);
    };

} //namespace view

#endif //__view_paint_lock_h__