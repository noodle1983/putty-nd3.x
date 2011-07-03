
#ifndef __view_focusable_border_h__
#define __view_focusable_border_h__

#pragma once

#include "view/border.h"
#include "view/view.h"

namespace gfx
{
    class Canvas;
    class Insets;
}

namespace view
{

    // A Border class to draw a focused border around a field (e.g textfield).
    class FocusableBorder : public Border
    {
    public:
        FocusableBorder();

        // Sets the insets of the border.
        void SetInsets(int top, int left, int bottom, int right);

        // Sets the focus state.
        void set_has_focus(bool has_focus)
        {
            has_focus_ = has_focus;
        }

        // Overridden from Border:
        virtual void Paint(const View& view, gfx::Canvas* canvas) const;
        virtual void GetInsets(gfx::Insets* insets) const;

    private:
        bool has_focus_;
        gfx::Insets insets_;

        DISALLOW_COPY_AND_ASSIGN(FocusableBorder);
    };

} //namespace view

#endif //__view_focusable_border_h__