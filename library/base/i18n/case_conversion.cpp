
#include "case_conversion.h"

#include <windows.h>

namespace base
{
    namespace i18n
    {

        string16 ToLower(const string16& string)
        {
            // WLW TODO: fix it.
            string16 lower(string);
            CharLowerW(const_cast<LPWSTR>(lower.c_str()));
            return lower;
        }

    } //namespace i18n
} //namespace base