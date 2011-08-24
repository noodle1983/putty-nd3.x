
#include "text_elider.h"

#include "base/logging.h"
#include "base/utf_string_conversions.h"

namespace ui
{

    // U+2026 in utf8
    const char kEllipsis[] = "\xE2\x80\xA6";

    namespace
    {

        // Cuts |text| to be |length| characters long.  If |cut_in_middle| is true, the
        // middle of the string is removed to leave equal-length pieces from the
        // beginning and end of the string; otherwise, the end of the string is removed
        // and only the beginning remains.  If |insert_ellipsis| is true, then an
        // ellipsis character will by inserted at the cut point.
        string16 CutString(const string16& text,
            size_t length,
            bool cut_in_middle,
            bool insert_ellipsis)
        {
            // TODO(tony): This is wrong, it might split the string in the middle of a
            // surrogate pair.
            const string16 kInsert = insert_ellipsis ? UTF8ToUTF16(kEllipsis) :
                ASCIIToUTF16("");
            if(!cut_in_middle)
            {
                return text.substr(0, length) + kInsert;
            }
            // We put the extra character, if any, before the cut.
            const size_t half_length = length / 2;
            return text.substr(0, length-half_length) + kInsert +
                text.substr(text.length()-half_length, half_length);
        }

    }

    // This function adds an ellipsis at the end of the text if the text
    // does not fit the given pixel width.
    string16 ElideText(const string16& text,
        const gfx::Font& font,
        int available_pixel_width,
        bool elide_in_middle)
    {
        if(text.empty())
        {
            return text;
        }

        int current_text_pixel_width = font.GetStringWidth(text);

        // Pango will return 0 width for absurdly long strings. Cut the string in
        // half and try again.
        // This is caused by an int overflow in Pango (specifically, in
        // pango_glyph_string_extents_range). It's actually more subtle than just
        // returning 0, since on super absurdly long strings, the int can wrap and
        // return positive numbers again. Detecting that is probably not worth it
        // (eliding way too much from a ridiculous string is probably still
        // ridiculous), but we should check other widths for bogus values as well.
        if(current_text_pixel_width<=0 && !text.empty())
        {
            return ElideText(CutString(text, text.length()/2, elide_in_middle, false),
                font, available_pixel_width, false);
        }

        if(current_text_pixel_width <= available_pixel_width)
        {
            return text;
        }

        if(font.GetStringWidth(UTF8ToUTF16(kEllipsis)) > available_pixel_width)
        {
            return string16();
        }

        // Use binary search to compute the elided text.
        size_t lo = 0;
        size_t hi = text.length() - 1;
        for(size_t guess=(lo+hi)/2; guess!=lo; guess=(lo+hi)/2)
        {
            // We check the length of the whole desired string at once to ensure we
            // handle kerning/ligatures/etc. correctly.
            int guess_length = font.GetStringWidth(CutString(text, guess,
                elide_in_middle, true));
            // Check again that we didn't hit a Pango width overflow. If so, cut the
            // current string in half and start over.
            if(guess_length <= 0)
            {
                return ElideText(CutString(text, guess/2, elide_in_middle, false),
                    font, available_pixel_width, elide_in_middle);
            }
            if(guess_length > available_pixel_width)
            {
                hi = guess;
            }
            else
            {
                lo = guess;
            }
        }

        return CutString(text, lo, elide_in_middle, true);
    }

    bool ElideString(const string16& input, int max_len, string16* output)
    {
        DCHECK_GE(max_len, 0);
        if(static_cast<int>(input.length()) <= max_len)
        {
            output->assign(input);
            return false;
        }

        switch(max_len)
        {
        case 0:
            output->clear();
            break;
        case 1:
            output->assign(input.substr(0, 1));
            break;
        case 2:
            output->assign(input.substr(0, 2));
            break;
        case 3:
            output->assign(input.substr(0, 1) + ASCIIToUTF16(".") +
                input.substr(input.length()-1));
            break;
        case 4:
            output->assign(input.substr(0, 1) + ASCIIToUTF16("..") +
                input.substr(input.length()-1));
            break;
        default:
            {
                int rstr_len = (max_len - 3) / 2;
                int lstr_len = rstr_len + ((max_len - 3) % 2);
                output->assign(input.substr(0, lstr_len) + ASCIIToUTF16("...") +
                    input.substr(input.length()-rstr_len));
                break;
            }
        }

        return true;
    }

} //namespace ui