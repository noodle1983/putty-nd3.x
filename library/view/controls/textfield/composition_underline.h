
#ifndef __view_composition_underline_h__
#define __view_composition_underline_h__

#pragma once

#include <vector>

#include "SkColor.h"

namespace view
{

    // Intentionally keep sync with WebKit::WebCompositionUnderline defined in:
    // third_party/WebKit/Source/WebKit/chromium/public/WebCompositionUnderline.h
    struct CompositionUnderline
    {
        CompositionUnderline()
            : start_offset(0),
            end_offset(0),
            color(0),
            thick(false) {}

        CompositionUnderline(unsigned s, unsigned e, SkColor c, bool t)
            : start_offset(s),
            end_offset(e),
            color(c),
            thick(t) {}

        // Though use of unsigned is discouraged, we use it here to make sure it's
        // identical to WebKit::WebCompositionUnderline.
        unsigned start_offset;
        unsigned end_offset;
        SkColor color;
        bool thick;
    };

    typedef std::vector<CompositionUnderline> CompositionUnderlines;

} //namespace view

#endif //__view_composition_underline_h__