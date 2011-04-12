
#ifndef __view_framework_background_h__
#define __view_framework_background_h__

#pragma once

#include <windows.h>

#include "base/basic_types.h"

#include "SkColor.h"

namespace gfx
{
    class Canvas;
}

namespace view
{

    class Painter;
    class View;

    /////////////////////////////////////////////////////////////////////////////
    //
    // Background class
    //
    // A background implements a way for views to paint a background. The
    // background can be either solid or based on a gradient. Of course,
    // Background can be subclassed to implement various effects.
    //
    // Any View can have a background. See View::SetBackground() and
    // View::PaintBackground()
    //
    /////////////////////////////////////////////////////////////////////////////
    class Background
    {
    public:
        Background();
        virtual ~Background();

        // Creates a background that fills the canvas in the specified color.
        static Background* CreateSolidBackground(const SkColor& color);

        // Creates a background that fills the canvas in the specified color.
        static Background* CreateSolidBackground(int r, int g, int b)
        {
            return CreateSolidBackground(SkColorSetRGB(r, g, b));
        }

        // Creates a background that fills the canvas in the specified color.
        static Background* CreateSolidBackground(int r, int g, int b, int a)
        {
            return CreateSolidBackground(SkColorSetARGB(a, r, g, b));
        }

        // Creates a background that contains a vertical gradient that varies
        // from |color1| to |color2|
        static Background* CreateVerticalGradientBackground(const SkColor& color1,
            const SkColor& color2);

        // Creates Chrome's standard panel background
        static Background* CreateStandardPanelBackground();

        // Creates a Background from the specified Painter. If owns_painter is
        // true, the Painter is deleted when the Border is deleted.
        static Background* CreateBackgroundPainter(bool owns_painter,
            Painter* painter);

        // Render the background for the provided view
        virtual void Paint(gfx::Canvas* canvas, View* view) const = 0;

        // Set a solid, opaque color to be used when drawing backgrounds of native
        // controls.  Unfortunately alpha=0 is not an option.
        void SetNativeControlColor(SkColor color);

        // Returns the "background color".  This is equivalent to the color set in
        // SetNativeControlColor().  For solid backgrounds, this is the color; for
        // gradient backgrounds, it's the midpoint of the gradient; for painter
        // backgrounds, this is not useful (returns a default color).
        SkColor get_color() const { return color_; }

        // TODO(port): Make GetNativeControlBrush portable (currently uses HBRUSH).

        // Get the brush that was specified by SetNativeControlColor
        HBRUSH GetNativeControlBrush() const { return native_control_brush_; };

    private:
        SkColor color_;
        // TODO(port): Create portable replacement for HBRUSH.
        HBRUSH native_control_brush_;

        DISALLOW_COPY_AND_ASSIGN(Background);
    };

} //namespace view

#endif //__view_framework_background_h__