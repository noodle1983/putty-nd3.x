
#ifndef __view_framework_text_elider_h__
#define __view_framework_text_elider_h__

#pragma once

#include "base/string16.h"
#include "base/file_path.h"

namespace gfx
{
    class Font;
}

namespace view
{

    extern const char kEllipsis[];

    // Elides |text| to fit in |available_pixel_width|.  If |elide_in_middle| is
    // set the ellipsis is placed in the middle of the string; otherwise it is
    // placed at the end.
    string16 ElideText(const string16& text,
        const gfx::Font& font,
        int available_pixel_width,
        bool elide_in_middle);

    // Functions to elide strings when the font information is unknown.  As
    // opposed to the above functions, the ElideString() and
    // ElideRectangleString() functions operate in terms of character units,
    // not pixels.

    // If the size of |input| is more than |max_len|, this function returns
    // true and |input| is shortened into |output| by removing chars in the
    // middle (they are replaced with up to 3 dots, as size permits).
    // Ex: ElideString(ASCIIToUTF16("Hello"), 10, &str) puts Hello in str and
    // returns false.  ElideString(ASCIIToUTF16("Hello my name is Tom"), 10, &str)
    // puts "Hell...Tom" in str and returns true.
    // TODO(tsepez): Doesn't handle UTF-16 surrogate pairs properly.
    // TODO(tsepez): Doesn't handle bidi properly.
    bool ElideString(const string16& input, int max_len, string16* output);

} //namespace view

#endif //__view_framework_text_elider_h__