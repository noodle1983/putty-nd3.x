
#ifndef __base_string_number_conversions_h__
#define __base_string_number_conversions_h__

#pragma once

#include <string>
#include <vector>

#include "basic_types.h"
#include "string16.h"

// ----------------------------------------------------------------------------
// 设计者的忠告
//
// 本文件不包含"wstring"版本, 新代码应该使用string16, 通过UTF8转换兼容老的代码.
// 不要添加"wstring"版本.
//
// 请别添加忽略成功/失败直接返回数值的字符串到整数转换所谓的"易用的"函数. 那样
// 会引导使用者编写不处理错误情况的代码.
// ----------------------------------------------------------------------------

namespace base
{

    // 数字 -> 字符串 转换 ------------------------------------------------
    std::string IntToString(int value);
    string16 IntToString16(int value);

    std::string UintToString(unsigned value);
    string16 UintToString16(unsigned value);

    std::string Int64ToString(int64 value);
    string16 Int64ToString16(int64 value);

    std::string Uint64ToString(uint64 value);
    string16 Uint64ToString16(uint64 value);

    std::string DoubleToString(double value);

    // 字符串 -> 数字 转换 ------------------------------------------------

    // 字符串转换为数字最有效的方式.
    // |*output|是转换的结果. 转换成功返回true; 以下情况返回false:
    // - 溢出/越界. |*output|被赋值为类型所支持的最大值.
    // - 数字字符后面带有其他字符. |*output|被赋值为解析数字部分的值.
    // - 字符串开头没有解析到数字字符. |*output|被赋值为0.
    // - 空字符串. |*output|被赋值为0.
    bool StringToInt(const std::string& input, int* output);
    bool StringToInt(std::string::const_iterator begin,
        std::string::const_iterator end, int* output);
    bool StringToInt(const char* begin, const char* end, int* output);

    bool StringToInt(const string16& input, int* output);
    bool StringToInt(string16::const_iterator begin,
        string16::const_iterator end, int* output);
    bool StringToInt(const char16* begin, const char16* end, int* output);

    bool StringToInt64(const std::string& input, int64* output);
    bool StringToInt64(std::string::const_iterator begin,
        std::string::const_iterator end, int64* output);
    bool StringToInt64(const char* begin, const char* end, int64* output);

    bool StringToInt64(const string16& input, int64* output);
    bool StringToInt64(string16::const_iterator begin,
        string16::const_iterator end, int64* output);
    bool StringToInt64(const char16* begin, const char16* end, int64* output);

    // 浮点数的转换只支持10进制格式, 不支持16进制和无限值(比如NaN).
    bool StringToDouble(const std::string& input, double* output);

    // 16进制编码 ----------------------------------------------------------------

    // 返回二进制缓冲区的16进制字符串, 大写方式表达. 函数不校验|size|长度的合理性.
    // 如果怀疑格式化的结果很大, |size|的最大值应该是
    //     std::numeric_limits<size_t>::max() / 2
    std::string HexEncode(const void* bytes, size_t size);

    // 注意StringToInt的限制.
    bool HexStringToInt(const std::string& input, int* output);
    bool HexStringToInt(std::string::const_iterator begin,
        std::string::const_iterator end, int* output);
    bool HexStringToInt(const char* begin, const char* end, int* output);

    // 与前一函数类似, 输出为bytes的vector. |*output|包含解析错误前尽可能多的字节.
    // 这里不会溢出, 但是input.size()必须是偶数. 0x或者+/-前缀是不允许的.
    bool HexStringToBytes(const std::string& input, std::vector<uint8>* output);

} //namespace base

#endif //__base_string_number_conversions_h__