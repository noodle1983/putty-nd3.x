
#ifndef __view_window_shape_h__
#define __view_window_shape_h__

#pragma once

namespace gfx
{
    class Size;
    class Path;
}

namespace view
{

    // Sets the window mask to a style that most likely matches
    // app/resources/window_*
    void GetDefaultWindowMask(const gfx::Size& size, gfx::Path* window_mask);

} //namespace view

#endif //__view_window_shape_h__