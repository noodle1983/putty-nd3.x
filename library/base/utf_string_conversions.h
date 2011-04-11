
#ifndef __base_utf_string_conversions_h__
#define __base_utf_string_conversions_h__

#pragma once

#include "string_piece.h"
#include "string16.h"

// UTF-8、-16和-32字符串之间的转换. 底层运算比较慢, 所以尽量避免不必要的转换.
// bool返回值表示转换是否100%合法, 尽可能多的进行转换输出. strings返回值的版本
// 忽略错误并返回可能的转换结果.
bool WideToUTF8(const wchar_t* src, size_t src_len, std::string* output);
std::string WideToUTF8(const std::wstring& wide);
bool UTF8ToWide(const char* src, size_t src_len, std::wstring* output);
std::wstring UTF8ToWide(const base::StringPiece& utf8);

bool WideToUTF16(const wchar_t* src, size_t src_len, string16* output);
string16 WideToUTF16(const std::wstring& wide);
bool UTF16ToWide(const char16* src, size_t src_len, std::wstring* output);
std::wstring UTF16ToWide(const string16& utf16);

bool UTF8ToUTF16(const char* src, size_t src_len, string16* output);
string16 UTF8ToUTF16(const std::string& utf8);
bool UTF16ToUTF8(const char16* src, size_t src_len, std::string* output);
std::string UTF16ToUTF8(const string16& utf16);

// 转化ASCII字符串(通常是字符串常量)到宽字节字符串.
std::wstring ASCIIToWide(const char* ascii);
std::wstring ASCIIToWide(const std::string& ascii);
string16 ASCIIToUTF16(const char* ascii);
string16 ASCIIToUTF16(const std::string& ascii);

#endif //__base_utf_string_conversions_h__