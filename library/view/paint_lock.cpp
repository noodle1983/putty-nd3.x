
#include "paint_lock.h"

#include "view.h"

namespace view
{

    PaintLock::PaintLock(View* view) : view_(view)
    {
        view_->set_painting_enabled(false);
    }

    PaintLock::~PaintLock()
    {
        view_->set_painting_enabled(true);
    }

} //namespace view