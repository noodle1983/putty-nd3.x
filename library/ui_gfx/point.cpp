
#include "point.h"

#include <windows.h>

#include <ostream>

namespace gfx
{

    Point::Point() : x_(0), y_(0) {}

    Point::Point(int x, int y) : x_(x), y_(y) {}

    Point::Point(DWORD point)
    {
        POINTS points = MAKEPOINTS(point);
        x_ = points.x;
        y_ = points.y;
    }

    Point::Point(const POINT& point) : x_(point.x), y_(point.y) {}

    Point& Point::operator=(const POINT& point)
    {
        x_ = point.x;
        y_ = point.y;
        return *this;
    }

    POINT Point::ToPOINT() const
    {
        POINT p;
        p.x = x_;
        p.y = y_;
        return p;
    }

    std::ostream& operator<<(std::ostream& out, const gfx::Point& p)
    {
        return out << p.x() << "," << p.y();
    }

} //namespace gfx