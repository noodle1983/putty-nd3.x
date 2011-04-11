
#ifndef __base_sys_string_conversions_h__
#define __base_sys_string_conversions_h__

#pragma once

#include <string>

#include "basic_types.h"

// 提供平台相关的字符串类型转换, 不使用ICU, 减少最小依赖.

namespace base
{

    class StringPiece;

    // 宽字节和UTF-8之间转换. 错误结果由系统定义.
    std::string SysWideToUTF8(const std::wstring& wide);
    std::wstring SysUTF8ToWide(const StringPiece& utf8);

    // 宽字节和多字节之间转换.
    // 危险: 会丢失或改变信息(Windows重启后).
    std::string SysWideToNativeMB(const std::wstring& wide);
    std::wstring SysNativeMBToWide(const StringPiece& native_mb);

    // 使用代码页进行8字节和宽字节之间转换. 代码页标识必须是Windows函数
    // MultiByteToWideChar()可接受的.
    std::wstring SysMultiByteToWide(const StringPiece& mb, uint32 code_page);
    std::string SysWideToMultiByte(const std::wstring& wide, uint32 code_page);

} //namespace base

#endif //__base_sys_string_conversions_h__