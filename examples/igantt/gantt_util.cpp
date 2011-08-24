
#include "gantt_util.h"

#include <algorithm>

gfx::Rect GetNormalizeRect(const gfx::Point& point1,
                           const gfx::Point& point2)
{
    gfx::Point origin(std::min(point1.x(), point2.x()),
        std::min(point1.y(), point2.y()));
    gfx::Size size(std::abs(point1.x()-point2.x()), 
        std::abs(point1.y()-point2.y()));

    return gfx::Rect(origin, size);
}