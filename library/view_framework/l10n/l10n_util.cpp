
#include "l10n_util.h"

#include "base/logging.h"
#include "base/stringprintf.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"

#include "../base/resource_bundle.h"

namespace view
{

    std::string GetStringUTF8(int message_id)
    {
        return UTF16ToUTF8(GetStringUTF16(message_id));
    }

    string16 GetStringUTF16(int message_id)
    {
        ResourceBundle& rb = ResourceBundle::GetSharedInstance();
        string16 str = rb.GetLocalizedString(message_id);

        return str;
    }

    static string16 GetStringF(int message_id,
        const std::vector<string16>& replacements,
        std::vector<size_t>* offsets)
    {
        // TODO(tc): We could save a string copy if we got the raw string as
        // a StringPiece and were able to call ReplaceStringPlaceholders with
        // a StringPiece format string and string16 substitution strings.  In
        // practice, the strings should be relatively short.
        ResourceBundle& rb = ResourceBundle::GetSharedInstance();
        const string16& format_string = rb.GetLocalizedString(message_id);

#ifndef NDEBUG
        // Make sure every replacement string is being used, so we don't just
        // silently fail to insert one. If |offsets| is non-NULL, then don't do this
        // check as the code may simply want to find the placeholders rather than
        // actually replacing them.
        if(!offsets)
        {
            std::string utf8_string = UTF16ToUTF8(format_string);

            // $9 is the highest allowed placeholder.
            for(size_t i=0; i<9; ++i)
            {
                bool placeholder_should_exist = replacements.size() > i;

                std::string placeholder = base::StringPrintf("$%d",
                    static_cast<int>(i+1));
                size_t pos = utf8_string.find(placeholder.c_str());
                if(placeholder_should_exist)
                {
                    DCHECK_NE(std::string::npos, pos) << " Didn't find a " <<
                        placeholder << " placeholder in " << utf8_string;
                }
                else
                {
                    DCHECK_EQ(std::string::npos, pos) << " Unexpectedly found a " <<
                        placeholder << " placeholder in " << utf8_string;
                }
            }
        }
#endif

        string16 formatted = ReplaceStringPlaceholders(format_string, replacements,
            offsets);

        return formatted;
    }

    string16 GetStringFUTF16(int message_id, const string16& a)
    {
        std::vector<string16> replacements;
        replacements.push_back(a);
        return GetStringF(message_id, replacements, NULL);
    }

    string16 ToLower(const string16& string)
    {
        string16 lower(string);
        CharLowerW(const_cast<LPWSTR>(lower.c_str()));
        return lower;
    }

} //namespace view