
#ifndef __base_utf_string_conversion_utils_h__
#define __base_utf_string_conversion_utils_h__

#pragma once

#include "basic_types.h"
#include "string16.h"

// 仅用于各种UTF字符串转换文件.

namespace base
{

    inline bool IsValidCodepoint(uint32 code_point)
    {
        // 不包含高代理码点([0xD800, 0xDFFF])和大于0x10FFFF(允许的最大码点)
        // 的码点.
        // 非字符和未指派码点是允许的.
        return code_point<0xD800u ||
            (code_point>=0xE000u && code_point<=0x10FFFFu);
    }

    inline bool IsValidCharacter(uint32 code_point)
    {
        // 合法码点集合中排除所有非字符(U+FDD0..U+FDEF, 和所有
        // 以0xFFFE结尾的码点).
        return code_point<0xD800u || (code_point>=0xE000u &&
            code_point<0xFDD0u) || (code_point>0xFDEFu &&
            code_point<=0x10FFFFu && (code_point&0xFFFEu)!=0xFFFEu);
    }

    // 读UTF-8流, |*code_point|输出下一个码点. |src|是待读串, |*char_index|是起始
    // 读取位置偏移, 会更新到最近读取的字符索引, 递增用于读取下一个字符.
    bool ReadUnicodeCharacter(const char* src, int32 src_len,
        int32* char_index, uint32* code_point);

    // 读取UTF-16字符, 作用类同上面8-bit版本.
    bool ReadUnicodeCharacter(const char16* src, int32 src_len,
        int32* char_index, uint32* code_point);

    // 添加一个UTF-8字符到给定的8-bit字符串. 返回写入的字节数.
    size_t WriteUnicodeCharacter(uint32 code_point, std::string* output);

    // 添加一个UTF-16字符到给定的16-bit字符串. 返回写入的16-bit值数量.
    size_t WriteUnicodeCharacter(uint32 code_point, string16* output);

    // 猜测UTF-8输出的字节长度, 清理output串并预留空间. 假定输入字符类型是
    // 无符号的, 这样对UTF-16和UTF-32都成立.
    template<typename CHAR>
    void PrepareForUTF8Output(const CHAR* src, size_t src_len, std::string* output);

    // 给定UTF-8输入, 预备UTF-16或者UTF-32输出缓冲区. 参见PrepareForUTF8Output().
    template<typename STRING>
    void PrepareForUTF16Or32Output(const char* src, size_t src_len, STRING* output);

} //namespace base

#endif //__base_utf_string_conversion_utils_h__