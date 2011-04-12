
#ifndef __view_framework_border_h__
#define __view_framework_border_h__

#pragma once

#include "base/basic_types.h"

#include "gfx/insets.h"

#include "SkColor.h"

namespace gfx
{
    class Canvas;
}

namespace view
{

    class View;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Border class.
    //
    // The border class is used to display a border around a view.
    // To set a border on a view, just call SetBorder on the view, for example:
    // view->set_border(Border::CreateSolidBorder(1, SkColorSetRGB(25, 25, 112));
    // Once set on a view, the border is owned by the view.
    //
    // IMPORTANT NOTE: not all views support borders at this point. In order to
    // support the border, views should make sure to use bounds excluding the
    // border (by calling View::GetLocalBoundsExcludingBorder) when doing layout and
    // painting.
    //
    ////////////////////////////////////////////////////////////////////////////////

    class Border
    {
    public:
        Border();
        virtual ~Border();

        // Creates a border that is a simple line of the specified thickness and
        // color.
        static Border* CreateSolidBorder(int thickness, SkColor color);

        // Creates a border for reserving space. The returned border does not
        // paint anything.
        static Border* CreateEmptyBorder(int top, int left, int bottom, int right);

        // Renders the border for the specified view.
        virtual void Paint(const View& view, gfx::Canvas* canvas) const = 0;

        // Sets the specified insets to the the border insets.
        virtual void GetInsets(gfx::Insets* insets) const = 0;

    private:
        DISALLOW_COPY_AND_ASSIGN(Border);
    };

} //namespace view

#endif //__view_framework_border_h__