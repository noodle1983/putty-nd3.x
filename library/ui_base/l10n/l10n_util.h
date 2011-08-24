
#ifndef __ui_base_l10n_util_h__
#define __ui_base_l10n_util_h__

#pragma once

#include "base/string16.h"

namespace ui
{

    // Pulls resource string from the string bundle and returns it.
    std::string GetStringUTF8(int message_id);
    string16 GetStringUTF16(int message_id);

    // Get a resource string and replace $1-$2-$3 with |a| and |b|
    // respectively.  Additionally, $$ is replaced by $.
    string16 GetStringFUTF16(int message_id, const string16& a);

} //namespace ui

#endif //__ui_base_l10n_util_h__