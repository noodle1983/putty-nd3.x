
#include "fill_layout.h"

#include "base/logging.h"

#include "view/view.h"

namespace view
{

    ///////////////////////////////////////////////////////////////////////////////
    // FillLayout

    FillLayout::FillLayout() {}

    FillLayout::~FillLayout() {}

    void FillLayout::Layout(View* host)
    {
        if(!host->has_children())
        {
            return;
        }

        View* frame_view = host->GetChildViewAt(0);
        frame_view->SetBounds(0, 0, host->width(), host->height());
    }

    gfx::Size FillLayout::GetPreferredSize(View* host)
    {
        DCHECK_EQ(1, host->child_count());
        return host->GetChildViewAt(0)->GetPreferredSize();
    }

} //namespace view