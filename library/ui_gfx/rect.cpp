
#include "rect.h"

#include <windows.h>
#include <ostream>

#include "insets.h"

namespace
{

    void AdjustAlongAxis(int dst_origin, int dst_size, int* origin, int* size)
    {
        if(*origin < dst_origin)
        {
            *origin = dst_origin;
            *size = std::min(dst_size, *size);
        }
        else
        {
            *size = std::min(dst_size, *size);
            *origin = std::min(dst_origin+dst_size, *origin+*size) - *size;
        }
    }

}

namespace gfx
{

    Rect::Rect() {}

    Rect::Rect(int width, int height) : size_(width, height) {}

    Rect::Rect(int x, int y, int width, int height)
        : origin_(x, y), size_(width, height) {}

    Rect::Rect(const gfx::Size& size) : size_(size) {}

    Rect::Rect(const gfx::Point& origin, const gfx::Size& size)
        : origin_(origin), size_(size) {}

    Rect::Rect(const RECT& r) : origin_(r.left, r.top)
    {
        set_width(r.right-r.left);
        set_height(r.bottom-r.top);
    }

    Rect& Rect::operator=(const RECT& r)
    {
        origin_.SetPoint(r.left, r.top);
        set_width(r.right - r.left);
        set_height(r.bottom - r.top);
        return *this;
    }

    void Rect::SetRect(int x, int y, int width, int height)
    {
        origin_.SetPoint(x, y);
        set_width(width);
        set_height(height);
    }

    void Rect::Inset(const gfx::Insets& insets)
    {
        Inset(insets.left(), insets.top(), insets.right(), insets.bottom());
    }

    void Rect::Inset(int left, int top, int right, int bottom)
    {
        Offset(left, top);
        set_width(std::max(width()-left-right, 0));
        set_height(std::max(height()-top-bottom, 0));
    }

    void Rect::Offset(int horizontal, int vertical)
    {
        origin_.Offset(horizontal, vertical);
    }

    bool Rect::operator==(const Rect& other) const
    {
        return origin_==other.origin_ && size_==other.size_;
    }

    bool Rect::operator<(const Rect& other) const
    {
        if(origin_ == other.origin_)
        {
            if(width() == other.width())
            {
                return height() < other.height();
            }
            else
            {
                return width() < other.width();
            }
        }
        else
        {
            return origin_ < other.origin_;
        }
    }

    RECT Rect::ToRECT() const
    {
        RECT r;
        r.left = x();
        r.right = right();
        r.top = y();
        r.bottom = bottom();
        return r;
    }

    bool Rect::Contains(int point_x, int point_y) const
    {
        return (point_x>=x()) && (point_x<right()) &&
            (point_y>=y()) && (point_y<bottom());
    }

    bool Rect::Contains(const Rect& rect) const
    {
        return (rect.x()>=x() && rect.right()<=right() &&
            rect.y()>=y() && rect.bottom()<=bottom());
    }

    bool Rect::Intersects(const Rect& rect) const
    {
        return !(rect.x()>=right() || rect.right()<=x() ||
            rect.y()>=bottom() || rect.bottom()<=y());
    }

    Rect Rect::Intersect(const Rect& rect) const
    {
        int rx = std::max(x(), rect.x());
        int ry = std::max(y(), rect.y());
        int rr = std::min(right(), rect.right());
        int rb = std::min(bottom(), rect.bottom());

        if(rx>=rr || ry>=rb)
        {
            rx = ry = rr = rb = 0; // 不相交.
        }

        return Rect(rx, ry, rr-rx, rb-ry);
    }

    Rect Rect::Union(const Rect& rect) const
    {
        // 空矩形特殊处理.
        if(IsEmpty())
        {
            return rect;
        }
        if(rect.IsEmpty())
        {
            return *this;
        }

        int rx = std::min(x(), rect.x());
        int ry = std::min(y(), rect.y());
        int rr = std::max(right(), rect.right());
        int rb = std::max(bottom(), rect.bottom());

        return Rect(rx, ry, rr-rx, rb-ry);
    }

    Rect Rect::Subtract(const Rect& rect) const
    {
        // 边界情况:
        if(!Intersects(rect))
        {
            return *this;
        }
        if(rect.Contains(*this))
        {
            return Rect();
        }

        int rx = x();
        int ry = y();
        int rr = right();
        int rb = bottom();

        if(rect.y()<=y() && rect.bottom()>=bottom())
        {
            // y方向完全相交.
            if(rect.x() <= x())
            {
                rx = rect.right();
            }
            else
            {
                rr = rect.x();
            }
        }
        else if(rect.x()<=x() && rect.right()>=right())
        {
            // x方向完全相交.
            if(rect.y() <= y())
            {
                ry = rect.bottom();
            }
            else
            {
                rb = rect.y();
            }
        }
        return Rect(rx, ry, rr-rx, rb-ry);
    }

    Rect Rect::AdjustToFit(const Rect& rect) const
    {
        int new_x = x();
        int new_y = y();
        int new_width = width();
        int new_height = height();
        AdjustAlongAxis(rect.x(), rect.width(), &new_x, &new_width);
        AdjustAlongAxis(rect.y(), rect.height(), &new_y, &new_height);
        return Rect(new_x, new_y, new_width, new_height);
    }

    Point Rect::CenterPoint() const
    {
        return Point(x()+(width()+1)/2, y()+(height()+1)/2);
    }

    Rect Rect::Center(const gfx::Size& size) const
    {
        int new_width = std::min(width(), size.width());
        int new_height = std::min(height(), size.height());
        int new_x = x() + (width() - new_width) / 2;
        int new_y = y() + (height() - new_height) / 2;
        return Rect(new_x, new_y, new_width, new_height);
    }

    bool Rect::SharesEdgeWith(const gfx::Rect& rect) const
    {
        return (y()==rect.y() && height()==rect.height() &&
            (x()==rect.right() || right()==rect.x())) ||
            (x()==rect.x() && width()==rect.width() &&
            (y()==rect.bottom() || bottom()==rect.y()));
    }

    std::ostream& operator<<(std::ostream& out, const gfx::Rect& r)
    {
        return out << r.origin() << " " << r.size();
    }

} //namespace gfx