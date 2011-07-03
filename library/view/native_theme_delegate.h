
#ifndef __view_native_theme_delegate_h__
#define __view_native_theme_delegate_h__

#pragma once

#include "ui_gfx/native_theme.h"
#include "ui_gfx/rect.h"

namespace ui
{
    class Animation;
}

namespace view
{

    // A delagate that supports animating transtions between different native
    // theme states.  This delegate can be used to control a native theme Border
    // or Painter object.
    //
    // If animation is onging, the native theme border or painter will
    // composite the foreground state over the backgroud state using an alpha
    // between 0 and 255 based on the current value of the animation.
    class NativeThemeDelegate
    {
    public:
        virtual ~NativeThemeDelegate() {}

        // Get the native theme part that should be drawn.
        virtual gfx::NativeTheme::Part GetThemePart() const = 0;

        // Get the rectangle that should be painted.
        virtual gfx::Rect GetThemePaintRect() const = 0;

        // Get the state of the part, along with any extra data needed for drawing.
        virtual gfx::NativeTheme::State GetThemeState(
            gfx::NativeTheme::ExtraParams* params) const = 0;

        // If the native theme drawign should be animated, return the Animation object
        // that controlls it.  If no animation is ongoing, NULL may be returned.
        virtual const ui::Animation* GetThemeAnimation() const = 0;

        // If animation is onging, this returns the background native theme state.
        virtual gfx::NativeTheme::State GetBackgroundThemeState(
            gfx::NativeTheme::ExtraParams* params) const = 0;

        // If animation is onging, this returns the foreground native theme state.
        // This state will be composited over the background using an alpha value
        // based on the current value of the animation.
        virtual gfx::NativeTheme::State GetForegroundThemeState(
            gfx::NativeTheme::ExtraParams* params) const = 0;
    };

} //namespace view

#endif //__view_native_theme_delegate_h__