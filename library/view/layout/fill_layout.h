
#ifndef __view_fill_layout_h__
#define __view_fill_layout_h__

#pragma once

#include "base/basic_types.h"

#include "layout_manager.h"

namespace view
{

    class View;

    ///////////////////////////////////////////////////////////////////////////////
    //
    // FillLayout
    //  A simple LayoutManager that causes the associated view's one child to be
    //  sized to match the bounds of its parent.
    //
    ///////////////////////////////////////////////////////////////////////////////
    class FillLayout : public LayoutManager
    {
    public:
        FillLayout();
        virtual ~FillLayout();

        // Overridden from LayoutManager:
        virtual void Layout(View* host);
        virtual gfx::Size GetPreferredSize(View* host);

    private:
        DISALLOW_COPY_AND_ASSIGN(FillLayout);
    };

} //namespace view

#endif //__view_fill_layout_h__