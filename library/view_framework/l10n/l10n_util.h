
#ifndef __view_framework_l10n_util_h__
#define __view_framework_l10n_util_h__

#pragma once

#include "base/string16.h"

namespace view
{

    // Pulls resource string from the string bundle and returns it.
    std::string GetStringUTF8(int message_id);
    string16 GetStringUTF16(int message_id);

    // Get a resource string and replace $1-$2-$3 with |a| and |b|
    // respectively.  Additionally, $$ is replaced by $.
    string16 GetStringFUTF16(int message_id, const string16& a);

    // Returns the lower case equivalent of string.
    string16 ToLower(const string16& string);

} //namespace view

#endif //__view_framework_l10n_util_h__