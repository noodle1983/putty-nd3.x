
#ifndef __base_string_split_h__
#define __base_string_split_h__

#pragma once

#include <vector>

#include "string16.h"

namespace base
{

    // 将|str|切分成以|s|分隔的子串向量组, 结果依次附加到|r|中.
    // 对于多个连续的|s|或者以|s|开头或者结尾的情况, 会加入一个空串.
    //
    // 每个子串开头和结尾的空白字符会被去除掉.
    // 当wchar_t是char16时候(比如Windows), |c|必须在基本多文种平面.
    // 其它地方(Linux/Mac), wchar_t应该是合法的Unicode code point(32-bit).
    void SplitString(const std::wstring& str,
        wchar_t c, std::vector<std::wstring>* r);
    // 注意: |c|必须在基本多文种平面.
    void SplitString(const std::string& str,
        char16 c, std::vector<std::string>* r);
    // |str|不应该使用多字节编码, 像Shift-JIS或者GBK那样多字节字符的尾字节
    // 可能在ASCII区间.
    // UTF-8和其它ASCII兼容的单/多字节编码是没问题的.
    // 注意: |c|必须在ASCII区间.
    void SplitString(const std::string& str,
        char c, std::vector<std::string>* r);

    bool SplitStringIntoKeyValues(const std::string& line,
        char key_value_delimiter, std::string* key,
        std::vector<std::string>* values);

    bool SplitStringIntoKeyValuePairs(const std::string& line,
        char key_value_delimiter, char key_value_pair_delimiter,
        std::vector<std::pair<std::string, std::string> >* kv_pairs);

    // 和SplitString一样, 只是用substring作为分隔符.
    void SplitStringUsingSubstr(const string16& str,
        const string16& s, std::vector<string16>* r);
    void SplitStringUsingSubstr(const std::string& str,
        const std::string& s, std::vector<std::string>* r);

    // 和SplitString一样, 只是不去除空白.
    // 注意: |c|必须在基本多文种平面.
    void SplitStringDontTrim(const string16& str,
        char16 c, std::vector<string16>* r);
    // |str|不应该使用多字节编码, 像Shift-JIS或者GBK那样多字节字符的尾字节
    // 可能在ASCII区间.
    // UTF-8和其它ASCII兼容的单/多字节编码是没问题的.
    // 注意: |c|必须在ASCII区间.
    void SplitStringDontTrim(const std::string& str,
        char c, std::vector<std::string>* r);

} //namespace base

#endif //__base_string_split_h__