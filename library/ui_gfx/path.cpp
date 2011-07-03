
#include "path.h"

#include "base/logging.h"

namespace gfx
{

    Path::Path() : SkPath()
    {
        moveTo(0, 0);
    }

    Path::Path(const Point* points, size_t count)
    {
        DCHECK(count > 1);
        moveTo(SkIntToScalar(points[0].x()), SkIntToScalar(points[0].y()));
        for(size_t i=1; i<count; ++i)
        {
            lineTo(SkIntToScalar(points[i].x()), SkIntToScalar(points[i].y()));
        }
    }

    Path::~Path() {}

} //namespace gfx