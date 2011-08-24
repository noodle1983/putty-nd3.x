
#include "native_theme.h"
#include "size.h"

namespace gfx
{

    unsigned int NativeTheme::thumb_inactive_color_ = 0xeaeaea;
    unsigned int NativeTheme::thumb_active_color_ = 0xf4f4f4;
    unsigned int NativeTheme::track_color_ = 0xd3d3d3;

    void NativeTheme::SetScrollbarColors(unsigned inactive_color,
        unsigned active_color, unsigned track_color) const
    {
        thumb_inactive_color_ = inactive_color;
        thumb_active_color_ = active_color;
        track_color_ = track_color;
    }

    // NativeTheme::instance() is implemented in native_theme_win.cpp.

} //namespace gfx