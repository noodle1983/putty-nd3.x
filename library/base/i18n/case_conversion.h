
#ifndef __base_case_conversion_h__
#define __base_case_conversion_h__

#pragma once

#include "base/string_piece.h"

namespace base
{
    namespace i18n
    {

        // Returns the lower case equivalent of string.
        string16 ToLower(const StringPiece16& string);

        // Returns the upper case equivalent of string.
        string16 ToUpper(const StringPiece16& string);

    } //namespace i18n
} //namespace base

#endif //__base_case_conversion_h__