
#include "size.h"

#include <windows.h>

#include <ostream>

#include "base/logging.h"

namespace gfx
{

    Size::Size(int width, int height)
    {
        set_width(width);
        set_height(height);
    }

    SIZE Size::ToSIZE() const
    {
        SIZE s;
        s.cx = width_;
        s.cy = height_;
        return s;
    }

    void Size::set_width(int width)
    {
        if(width < 0)
        {
            NOTREACHED() << "negative width:" << width;
            width = 0;
        }
        width_ = width;
    }

    void Size::set_height(int height)
    {
        if(height < 0)
        {
            NOTREACHED() << "negative height:" << height;
            height = 0;
        }
        height_ = height;
    }

    std::ostream& operator<<(std::ostream& out, const gfx::Size& s)
    {
        return out << s.width() << "x" << s.height();
    }

} //namespace gfx