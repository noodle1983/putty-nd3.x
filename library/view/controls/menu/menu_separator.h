
#ifndef __view_framework_menu_separator_h__
#define __view_framework_menu_separator_h__

#pragma once

#include "../../view/view.h"

namespace view
{

    class MenuSeparator : public View
    {
    public:
        MenuSeparator() {}

        // View overrides.
        virtual void OnPaint(gfx::Canvas* canvas);
        virtual gfx::Size GetPreferredSize();

    private:
        DISALLOW_COPY_AND_ASSIGN(MenuSeparator);
    };

} //namespace view

#endif //__view_framework_menu_separator_h__