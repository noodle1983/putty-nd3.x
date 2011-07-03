
#ifndef __base_algorithm_string_escape_h__
#define __base_algorithm_string_escape_h__

#pragma once

#include <string>

#include "base/string16.h"

namespace base
{

    // 转义编码|str|成合法的JSON字符, 附加结果到|dst|, 创建统一的转义字符序列.
    // 如果|put_in_quotes|为true, 用""把结果括起来. 输出的文本, 经浏览器翻译成
    // javascript的字符串和输入的|str|完全相同.
    void JsonDoubleQuote(const std::string& str,
        bool put_in_quotes, std::string* dst);

    // 和上面的函数一样, 但始终用""把返回的结果括起来.
    std::string GetDoubleQuotedJson(const std::string& str);

    void JsonDoubleQuote(const string16& str,
        bool put_in_quotes, std::string* dst);

    // 和上面的函数一样, 但始终用""把返回的结果括起来.
    std::string GetDoubleQuotedJson(const string16& str);

} //namespace base

#endif //__base_algorithm_string_escape_h__