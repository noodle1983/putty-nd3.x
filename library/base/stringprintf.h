
#ifndef __base_stringprintf_h__
#define __base_stringprintf_h__

#pragma once

#include <stdarg.h>

#include <string>

namespace base
{

    std::string StringPrintf(const char* format, ...);
    std::wstring StringPrintf(const wchar_t* format, ...);

    std::string StringPrintV(const char* format, va_list ap);

    const std::string& SStringPrintf(std::string* dst,
        const char* format, ...);
    const std::wstring& SStringPrintf(std::wstring* dst,
        const wchar_t* format, ...);

    void StringAppendF(std::string* dst, const char* format, ...);
    void StringAppendF(std::wstring* dst, const wchar_t* format, ...);

    void StringAppendV(std::string* dst, const char* format, va_list ap);
    void StringAppendV(std::wstring* dst, const wchar_t* format, va_list ap);

} //namespace base

#endif //__base_stringprintf_h__