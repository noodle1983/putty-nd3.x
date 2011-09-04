
#include "rtl.h"

namespace
{

    // define some other languages until ntdefs.h catches up
#ifndef LANG_NAPALI
#define LANG_NAPALI     0x61
#endif
#ifndef LANG_BURMESE
#define LANG_BURMESE    0x55 // Burma
#endif
#ifndef LANG_YIDDISH
#define LANG_YIDDISH    0x3d
#endif

    bool IsRTLLang(LANGID lang)
    {
        switch(lang)
        {
        case LANG_ARABIC:
        case LANG_HEBREW:
        case LANG_URDU:
        case LANG_FARSI:
        case LANG_YIDDISH:
        case LANG_SINDHI:
        case LANG_KASHMIRI:
            return true;
        }

        return false;
    }

    bool IsRtlLCID(LCID lcid)
    {
        return IsRTLLang(PRIMARYLANGID(LANGIDFROMLCID(lcid)));
    }

#define IN_RANGE(n1, b, n2) ((unsigned)((b)-n1)<=n2-n1)

    inline BOOL IsRTLCharCore(WCHAR ch)
    {
        // Bitmask of RLM, RLE, RLO, based from RLM in the 0 bit.
#define MASK_RTLPUNCT   0x90000001

        return (IN_RANGE(0x0590, ch, 0x08ff) || // In RTL block
            (IN_RANGE(0x200f, ch, 0x202e) &&    // Possible RTL punct char
            ((MASK_RTLPUNCT>>(ch-0x200f))&1))); // Mask of RTL punct chars
    }

    inline BOOL IsRTLChar(WCHAR ch)
    {
        return (IN_RANGE(0x0590/* First RTL char */, ch, 0x202e/* RLO */) &&
            IsRTLCharCore(ch));
    }

}

namespace base
{
    namespace i18n
    {

        bool IsRTL()
        {
            return IsRtlLCID(GetThreadLocale());
        }

        bool AdjustStringForLocaleDirection(string16* text)
        {
            if(!IsRTL() || text->empty())
            {
                return false;
            }

            // Marking the string as LTR if the locale is RTL and the string does not
            // contain strong RTL characters. Otherwise, mark the string as RTL.
            bool has_rtl_chars = StringContainsStrongRTLChars(*text);
            if(!has_rtl_chars)
            {
                WrapStringWithLTRFormatting(text);
            }
            else
            {
                WrapStringWithRTLFormatting(text);
            }

            return true;
        }

        bool StringContainsStrongRTLChars(const string16& text)
        {
            size_t length = text.length();
            for(size_t i=0; i<length; ++i)
            {
                if(IsRTLChar(text[i]))
                {
                    return true;
                }
            }
            return false;
        }

        void WrapStringWithLTRFormatting(string16* text)
        {
            if(text->empty())
            {
                return;
            }

            // Inserting an LRE (Left-To-Right Embedding) mark as the first character.
            text->insert(0U, 1U, kLeftToRightEmbeddingMark);

            // Inserting a PDF (Pop Directional Formatting) mark as the last character.
            text->push_back(kPopDirectionalFormatting);
        }

        void WrapStringWithRTLFormatting(string16* text)
        {
            if(text->empty())
            {
                return;
            }

            // Inserting an RLE (Right-To-Left Embedding) mark as the first character.
            text->insert(0U, 1U, kRightToLeftEmbeddingMark);

            // Inserting a PDF (Pop Directional Formatting) mark as the last character.
            text->push_back(kPopDirectionalFormatting);
        }

        string16 GetDisplayStringInLTRDirectionality(const string16& text)
        {
            if(!IsRTL())
            {
                return text;
            }
            string16 text_mutable(text);
            WrapStringWithLTRFormatting(&text_mutable);
            return text_mutable;
        }

        const string16 StripWrappingBidiControlCharacters(const string16& text)
        {
            if(text.empty())
            {
                return text;
            }
            size_t begin_index = 0;
            char16 begin = text[begin_index];
            if(begin==kLeftToRightEmbeddingMark ||
                begin==kRightToLeftEmbeddingMark ||
                begin==kLeftToRightOverride ||
                begin==kRightToLeftOverride)
            {
                ++begin_index;
            }
            size_t end_index = text.length() - 1;
            if(text[end_index] == kPopDirectionalFormatting)
            {
                --end_index;
            }
            return text.substr(begin_index, end_index-begin_index+1);
        }

    } //namespace i18n
} //namespace base