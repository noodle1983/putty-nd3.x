
#ifndef __ui_gfx_scrollbar_size_h__
#define __ui_gfx_scrollbar_size_h__

#pragma once

namespace gfx
{

    // This should return the thickness, in pixels, of a scrollbar in web content.
    // This needs to match the values in WebCore's
    // ScrollbarThemeChromiumXXX.cpp::scrollbarThickness().
    int scrollbar_size();

} //namespace gfx

#endif //__ui_gfx_scrollbar_size_h__