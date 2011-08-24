
#include "case_conversion.h"

#include <windows.h>

namespace base
{
    namespace i18n
    {

        string16 ToLower(const StringPiece16& string)
        {
            // WLW TODO: fix it.
            string16 lower(string.data());
            CharLowerW(const_cast<LPWSTR>(lower.c_str()));
            return lower;
        }

        string16 ToUpper(const StringPiece16& string)
        {
            // WLW TODO: fix it.
            string16 upper(string.data());
            CharUpperW(const_cast<LPWSTR>(upper.c_str()));
            return upper;
        }

    } //namespace i18n
} //namespace base