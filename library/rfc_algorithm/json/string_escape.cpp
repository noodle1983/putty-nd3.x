
#include "string_escape.h"

#include "base/string_util.h"
#include "base/stringprintf.h"

namespace
{

    // 尝试对|c|进行转义(\n等). 成功返回true并将转义字符串添加到|dst|. 规范没有
    // 强制规定, 但这样比\uXXXX方式更具可读性.
    template<typename CHAR>
    static bool JsonSingleEscapeChar(const CHAR c, std::string* dst)
    {
        // 警告: 这里如果添加新的转义, 需要同时更新reader.
        // 注意: \v在reader中, JSON规范不允许所以没出现在这里.
        switch(c)
        {
        case '\b':
            dst->append("\\b");
            break;
        case '\f':
            dst->append("\\f");
            break;
        case '\n':
            dst->append("\\n");
            break;
        case '\r':
            dst->append("\\r");
            break;
        case '\t':
            dst->append("\\t");
            break;
        case '\\':
            dst->append("\\\\");
            break;
        case '"':
            dst->append("\\\"");
            break;
        default:
            return false;
        }
        return true;
    }

    template<class STR>
    void JsonDoubleQuoteT(const STR& str, bool put_in_quotes, std::string* dst)
    {
        if(put_in_quotes)
        {
            dst->push_back('"');
        }

        for(typename STR::const_iterator it=str.begin(); it!=str.end(); ++it)
        {
            typename ToUnsigned<typename STR::value_type>::Unsigned c = *it;
            if(!JsonSingleEscapeChar(c, dst))
            {
                if(c<32 || c>126 || c=='<' || c=='>')
                {
                    unsigned int as_uint = static_cast<unsigned int>(c);
                    // 1. 对<、>转义防止脚本执行.
                    // 2. 技术上来讲, 可以把c>126的字符处理成UTF8, 这是可选方案.
                    //    这里实现起来不是那么简单.
                    base::StringAppendF(dst, "\\u%04X", as_uint);
                }
                else
                {
                    unsigned char ascii = static_cast<unsigned char>(*it);
                    dst->push_back(ascii);
                }
            }
        }

        if(put_in_quotes)
        {
            dst->push_back('"');
        }
    }

}

namespace base
{

    void JsonDoubleQuote(const std::string& str,
        bool put_in_quotes, std::string* dst)
    {
        JsonDoubleQuoteT(str, put_in_quotes, dst);
    }

    std::string GetDoubleQuotedJson(const std::string& str)
    {
        std::string dst;
        JsonDoubleQuote(str, true, &dst);
        return dst;
    }

    void JsonDoubleQuote(const string16& str,
        bool put_in_quotes, std::string* dst)
    {
        JsonDoubleQuoteT(str, put_in_quotes, dst);
    }

    std::string GetDoubleQuotedJson(const string16& str)
    {
        std::string dst;
        JsonDoubleQuote(str, true, &dst);
        return dst;
    }

} //namespace base