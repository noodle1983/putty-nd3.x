
#ifndef __base_rtl_h__
#define __base_rtl_h__

#include <windows.h>

#include "string16.h"

namespace base
{

    const char16 kRightToLeftMark = 0x200F;
    const char16 kLeftToRightMark = 0x200E;
    const char16 kLeftToRightEmbeddingMark = 0x202A;
    const char16 kRightToLeftEmbeddingMark = 0x202B;
    const char16 kPopDirectionalFormatting = 0x202C;
    const char16 kLeftToRightOverride = 0x202D;
    const char16 kRightToLeftOverride = 0x202E;

    enum TextDirection
    {
        UNKNOWN_DIRECTION,
        RIGHT_TO_LEFT,
        LEFT_TO_RIGHT,
    };

    // Returns true if the application text direction is right-to-left.
    bool IsRTL();

    // Given the string in |text|, this function modifies the string in place with
    // the appropriate Unicode formatting marks that mark the string direction
    // (either left-to-right or right-to-left). The function checks both the current
    // locale and the contents of the string in order to determine the direction of
    // the returned string. The function returns true if the string in |text| was
    // properly adjusted.
    //
    // Certain LTR strings are not rendered correctly when the context is RTL. For
    // example, the string "Foo!" will appear as "!Foo" if it is rendered as is in
    // an RTL context. Calling this function will make sure the returned localized
    // string is always treated as a right-to-left string. This is done by
    // inserting certain Unicode formatting marks into the returned string.
    //
    // ** Notes about the Windows version of this function:
    // TODO(idana) bug 6806: this function adjusts the string in question only
    // if the current locale is right-to-left. The function does not take care of
    // the opposite case (an RTL string displayed in an LTR context) since
    // adjusting the string involves inserting Unicode formatting characters that
    // Windows does not handle well unless right-to-left language support is
    // installed. Since the English version of Windows doesn't have right-to-left
    // language support installed by default, inserting the direction Unicode mark
    // results in Windows displaying squares.
    bool AdjustStringForLocaleDirection(string16* text);

    // Returns true if the string contains at least one character with strong right
    // to left directionality; that is, a character with either R or AL Unicode
    // BiDi character type.
    bool StringContainsStrongRTLChars(const string16& text);

    // Wraps a string with an LRE-PDF pair which essentialy marks the string as a
    // Left-To-Right string. Doing this is useful in order to make sure LTR
    // strings are rendered properly in an RTL context.
    void WrapStringWithLTRFormatting(string16* text);

    // Wraps a string with an RLE-PDF pair which essentialy marks the string as a
    // Right-To-Left string. Doing this is useful in order to make sure RTL
    // strings are rendered properly in an LTR context.
    void WrapStringWithRTLFormatting(string16* text);

    // Given the string in |text|, this function returns the adjusted string having
    // LTR directionality for display purpose. Which means that in RTL locale the
    // string is wrapped with LRE (Left-To-Right Embedding) and PDF (Pop
    // Directional Formatting) marks and returned. In LTR locale, the string itself
    // is returned.
    string16 GetDisplayStringInLTRDirectionality(const string16& text);

    // Strip the beginning (U+202A..U+202B, U+202D..U+202E) and/or ending (U+202C)
    // explicit bidi control characters from |text|, if there are any. Otherwise,
    // return the text itself. Explicit bidi control characters display and have
    // semantic effect. They can be deleted so they might not always appear in a
    // pair.
    const string16 StripWrappingBidiControlCharacters(const string16& text);

} //namespace base

#endif //__base_rtl_h__