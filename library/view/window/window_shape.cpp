
#include "window_shape.h"

#include "ui_gfx/path.h"
#include "ui_gfx/size.h"

namespace view
{

    void GetDefaultWindowMask(const gfx::Size& size, gfx::Path* window_mask)
    {
        // Redefine the window visible region for the new size.
        window_mask->moveTo(0, 3);
        window_mask->lineTo(1, 2);
        window_mask->lineTo(1, 1);
        window_mask->lineTo(2, 1);
        window_mask->lineTo(3, 0);

        window_mask->lineTo(SkIntToScalar(size.width()-3), 0);
        window_mask->lineTo(SkIntToScalar(size.width()-2), 1);
        window_mask->lineTo(SkIntToScalar(size.width()-1), 1);
        window_mask->lineTo(SkIntToScalar(size.width()-1), 2);
        window_mask->lineTo(SkIntToScalar(size.width()), 3);

        window_mask->lineTo(SkIntToScalar(size.width()),
            SkIntToScalar(size.height()-3));
        window_mask->lineTo(SkIntToScalar(size.width()-1),
            SkIntToScalar(size.height()-3));
        window_mask->lineTo(SkIntToScalar(size.width()-1),
            SkIntToScalar(size.height()-1));
        window_mask->lineTo(SkIntToScalar(size.width()-3),
            SkIntToScalar(size.height()-2));
        window_mask->lineTo(SkIntToScalar(size.width()-3),
            SkIntToScalar(size.height()));

        window_mask->lineTo(3, SkIntToScalar(size.height()));
        window_mask->lineTo(2, SkIntToScalar(size.height()-2));
        window_mask->lineTo(1, SkIntToScalar(size.height()-1));
        window_mask->lineTo(1, SkIntToScalar(size.height()-3));
        window_mask->lineTo(0, SkIntToScalar(size.height()-3));

        window_mask->close();
    }

} //namespace view