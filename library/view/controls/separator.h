
#ifndef __view_separator_h__
#define __view_separator_h__

#pragma once

#include <string>

#include "view/view.h"

namespace view
{

    // The Separator class is a view that shows a line used to visually separate
    // other views.  The current implementation is only horizontal.

    class Separator : public View
    {
    public:
        // The separator's class name.
        static const char kViewClassName[];

        Separator();
        virtual ~Separator();

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void GetAccessibleState(ui::AccessibleViewState* state);
        virtual void Paint(gfx::Canvas* canvas);
        virtual std::string GetClassName() const;

    private:
        DISALLOW_COPY_AND_ASSIGN(Separator);
    };

} //namespace view

#endif //__view_separator_h__