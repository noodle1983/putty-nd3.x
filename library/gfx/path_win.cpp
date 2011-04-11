
#include "path.h"

#include "base/scoped_ptr.h"

namespace gfx
{

    HRGN Path::CreateNativeRegion() const
    {
        int point_count = getPoints(NULL, 0);
        scoped_array<SkPoint> points(new SkPoint[point_count]);
        getPoints(points.get(), point_count);
        scoped_array<POINT> windows_points(new POINT[point_count]);
        for(int i=0; i<point_count; ++i)
        {
            windows_points[i].x = SkScalarRound(points[i].fX);
            windows_points[i].y = SkScalarRound(points[i].fY);
        }

        return ::CreatePolygonRgn(windows_points.get(), point_count, ALTERNATE);
    }

    // static
    HRGN Path::IntersectRegions(HRGN r1, HRGN r2)
    {
        HRGN dest = CreateRectRgn(0, 0, 1, 1);
        CombineRgn(dest, r1, r2, RGN_AND);
        return dest;
    }

    // static
    HRGN Path::CombineRegions(HRGN r1, HRGN r2)
    {
        HRGN dest = CreateRectRgn(0, 0, 1, 1);
        CombineRgn(dest, r1, r2, RGN_OR);
        return dest;
    }

    // static
    HRGN Path::SubtractRegion(HRGN r1, HRGN r2)
    {
        HRGN dest = CreateRectRgn(0, 0, 1, 1);
        CombineRgn(dest, r1, r2, RGN_DIFF);
        return dest;
    }

} //namespace gfx