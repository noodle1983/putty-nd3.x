
#ifndef __view_native_theme_painter_h__
#define __view_native_theme_painter_h__

#pragma once

#include "painter.h"

namespace gfx
{
    class Canvas;
    class Size;
}

namespace view
{

    class NativeThemeDelegate;

    // A Painter that uses NativeTheme to implement painting and sizing.  A
    // theme delegate must be given at construction time so that the appropriate
    // painting and sizing can be done.
    class NativeThemePainter : public Painter
    {
    public:
        explicit NativeThemePainter(NativeThemeDelegate* delegate);

        virtual ~NativeThemePainter() {}

        // Returns the preferred size of the native part being painted.
        gfx::Size GetPreferredSize();

    private:
        // The delegate the controls the appearance of this painter.
        NativeThemeDelegate* delegate_;

        // Overridden from Painter:
        virtual void Paint(int w, int h, gfx::Canvas* canvas);

        DISALLOW_COPY_AND_ASSIGN(NativeThemePainter);
    };

} //namespace view

#endif //__view_native_theme_painter_h__