
#ifndef __base_string_util_h__
#define __base_string_util_h__

#pragma once

#include <stdarg.h> 

#include <string>
#include <vector>

#include "logging.h"
#include "string16.h"
#include "string_piece.h"

// 定义字符串操作的实用函数.

namespace base
{

    // 封装类似"strncasecmp"和"snprintf"这种不是跨平台的标准c函数. 函数实现通过
    // 内联调用平台相关的函数.

    // 以大小写无关的方式比较两个字符串s1和s2; 按照字典型方式比较, 相等返回0,
    // s1>s2返回1, s2>s1返回-1.
    int strcasecmp(const char* s1, const char* s2);

    // 以大小写无关的方式比较两个字符串s1和s2前count个字符; 按照字典型方式比较,
    // 相等返回0, s1>s2返回1, s2>s1返回-1.
    int strncasecmp(const char* s1, const char* s2, size_t count);

    // char16版本的strncmp.
    int strncmp16(const char16* s1, const char16* s2, size_t count);

    // 封装vsnprintf保证字符串始终以null结尾并返回字符长度, 即使发生截断时也一样.
    int vsnprintf(char* buffer, size_t size, const char* format, va_list arguments);

    // vsnprintf_s字符串始终以null结尾, 当发生截断时, 返回-1或者不截断格式化的字符数,
    // 实际值依赖标准C库的vswprintf底层实现.
    int vswprintf(wchar_t* buffer, size_t size, const wchar_t* format,
        va_list arguments);

    inline int snprintf(char* buffer, size_t size, const char* format, ...)
    {
        va_list arguments;
        va_start(arguments, format);
        int result = vsnprintf(buffer, size, format, arguments);
        va_end(arguments);
        return result;
    }

    inline int swprintf(wchar_t* buffer, size_t size, const wchar_t* format, ...)
    {
        va_list arguments;
        va_start(arguments, format);
        int result = vswprintf(buffer, size, format, arguments);
        va_end(arguments);
        return result;
    }

    // BSD风格的字符串拷贝函数.
    // 拷贝|src|到|dst|, |dst_size|是|dst|长度. 最多拷贝|dst_size|-1个字符,
    // 只要|dst_size|不是0, |dst|以NULL结尾. 返回|src|的字符串长度.
    // 如果返回值>=dst_size，输出发生截断.
    // 注意: 长度都是字符个数而不是字节数.
    size_t strlcpy(char* dst, const char* src, size_t dst_size);
    size_t wcslcpy(wchar_t* dst, const wchar_t* src, size_t dst_size);

    // 扫描wprintf的格式串确定是否平台兼容. 函数只检查大多数平台支持的转换控制符,
    // 不检查格式串的其它错误.
    //
    // wprintf不可移植的转换控制符有:
    //  - 没有'l'长度修饰符的's'和'c'. %s和%c转换char数据, 除了Windows平台以外(
    //    转换wchar_t数据), %ls和%lc转换wchar_t数据.
    //  - 'S'和'C'转换wchar_t数据, 除了Windows平台以外(转换char数据). Windows平台
    //    使用%ls和%lc转换wchar_t数据.
    //  - Windows平台的wprintf不识别'F'.
    //  - 'D', 'O'和'U'已经被弃用, 使用%ld, %lo和%lu替换.
    //
    // 注意使用wprintf时char数据没有可移植的转换控制符.
    //
    // 函数在base::vswprintf中使用.
    bool IsWprintfFormatPortable(const wchar_t* format);

} // namespace base

namespace base
{

    // 自己的代码中不要使用malloc分配字符串; 这里仅用于配合APIs.
    inline char* strdup(const char* str)
    {
        return _strdup(str);
    }

    inline int strcasecmp(const char* s1, const char* s2)
    {
        return _stricmp(s1, s2);
    }

    inline int strncasecmp(const char* s1, const char* s2, size_t count)
    {
        return _strnicmp(s1, s2, count);
    }

    inline int strncmp16(const char16* s1, const char16* s2, size_t count)
    {
        return ::wcsncmp(s1, s2, count);
    }

    inline int vsnprintf(char* buffer, size_t size, const char* format,
        va_list arguments)
    {
        int length = vsnprintf_s(buffer, size, size-1, format, arguments);
        if(length < 0)
        {
            return _vscprintf(format, arguments);
        }
        return length;
    }

    inline int vswprintf(wchar_t* buffer, size_t size, const wchar_t* format,
        va_list arguments)
    {
        DCHECK(IsWprintfFormatPortable(format));

        int length = _vsnwprintf_s(buffer, size, size-1, format, arguments);
        if(length < 0)
        {
            return _vscwprintf(format, arguments);
        }
        return length;
    }

} // namespace base

// 这些线程安全的函数返回全局同一个空字符串.
//
// 一般情况下不要用于构造函数. 只有一种使用特例: 函数需要返回一个字符串引用(
// 比如成员函数), 又没有空字符串可用时(比如错误情况下). 不要用于函数参数的 
// 默认值或者当做函数的返回值.
const std::string& EmptyString();
const std::wstring& EmptyWString();
const string16& EmptyString16();

extern const wchar_t kWhitespaceWide[];
extern const char16 kWhitespaceUTF16[];
extern const char kWhitespaceASCII[];

extern const char kUtf8ByteOrderMark[];

// 从input移除remove_chars中所有的字符. 如果移除了字符则返回true.
// 注意: input和output使用同一变量是安全的.
bool RemoveChars(const std::wstring& input, const wchar_t remove_chars[],
                 std::wstring* output);
bool RemoveChars(const string16& input, const char16 remove_chars[],
                 string16* output);
bool RemoveChars(const std::string& input, const char remove_chars[],
                 std::string* output);

// 从input两端移除trim_chars中所有的字符.
// 注意: input和output使用同一变量是安全的.
bool TrimString(const std::wstring& input, const wchar_t trim_chars[],
                std::wstring* output);
bool TrimString(const string16& input, const char16 trim_chars[],
                string16* output);
bool TrimString(const std::string& input, const char trim_chars[],
                std::string* output);

// 字符串截断至接近byte_size字节大小.
void TruncateUTF8ToByteSize(const std::string& input, const size_t byte_size,
                            std::string* output);

// 指定去除input两端的空白字符, 并返回两端空白情况.
// 非宽字符版本有2个函数:
// * TrimWhitespace(const std::string& ...)
//   这个函数处理ASCII字符串, 只识别ASCII空白字符.
// 根据需要选择不同函数, 输入输出类型保持一致以保证安全.
// 注意: input和output使用同一变量是安全的.
enum TrimPositions
{
    TRIM_NONE     = 0,
    TRIM_LEADING  = 1 << 0,
    TRIM_TRAILING = 1 << 1,
    TRIM_ALL      = TRIM_LEADING | TRIM_TRAILING,
};
TrimPositions TrimWhitespace(const std::wstring& input, TrimPositions positions,
                             std::wstring* output);
TrimPositions TrimWhitespace(const string16& input, TrimPositions positions,
                             string16* output);
TrimPositions TrimWhitespace(const std::string& input, TrimPositions positions,
                             std::string* output);
TrimPositions TrimWhitespaceASCII(const std::string& input, TrimPositions positions,
                                  std::string* output);

// 查找CR或者LF字符. 移除所有包含CR或者LF的连续空白. 用于处理终端拷贝的字符.
// 返回的|text|经过以下三种转换:
// (1) 移除开头和结尾的空白.
// (2) 如果|trim_sequences_with_line_breaks|为真, 移除所有包含CR或者LF的连续空白.
// (3) 其它的连续空白字符转换成一个空白字符.
std::wstring CollapseWhitespace(const std::wstring& text,
                                bool trim_sequences_with_line_breaks);
string16 CollapseWhitespace(const string16& text,
                            bool trim_sequences_with_line_breaks);
std::string CollapseWhitespaceASCII(const std::string& text,
                                    bool trim_sequences_with_line_breaks);

// 如果字符串为空或者只含有空白字符则返回true.
bool ContainsOnlyWhitespaceASCII(const std::string& str);
bool ContainsOnlyWhitespace(const string16& str);

// 如果|input|为空或者只含有|characters|中的字符则返回true.
bool ContainsOnlyChars(const std::wstring& input, const std::wstring& characters);
bool ContainsOnlyChars(const string16& input, const string16& characters);
bool ContainsOnlyChars(const std::string& input, const std::string& characters);

// 通过截断方式转化到7-bit ASCII码, 需要预先知道转化前内容都是ASCII码.
std::string WideToASCII(const std::wstring& wide);
std::string UTF16ToASCII(const string16& utf16);

// 转换宽字符到相应的Latin1. 任意字符大于255则转换失败(返回false).
bool WideToLatin1(const std::wstring& wide, std::string* latin1);

// 成功返回true. 如何识别一个宽字符是8-bit还是UTF8? 前者只包含<256的字符;
// 后者也只使用8-bit字符但可能表示UTF-8字符串.
//
// IsStringUTF8不仅可以识别输入是否合法, 还能判断是否有非ASCII的字码(例如 U+FFFE).
// 这样做是为了让调用者能尽量识别其它编码. 如果有只需要识别输入是否合法需求,
// 可以新加一个函数.
bool IsStringUTF8(const std::string& str);
bool IsStringASCII(const std::wstring& str);
bool IsStringASCII(const base::StringPiece& str);
bool IsStringASCII(const string16& str);

// ASCII特定的tolower.
template<class Char>
inline Char ToLowerASCII(Char c)
{
    return (c>='A' && c<='Z') ? (c+('a'-'A')) : c;
}

template<class str>
inline void StringToLowerASCII(str* s)
{
    for(typename str::iterator i=s->begin(); i!=s->end(); ++i)
    {
        *i = ToLowerASCII(*i);
    }
}

template<class str>
inline str StringToLowerASCII(const str& s)
{
    // 针对std::string和std::wstring
    str output(s);
    StringToLowerASCII(&output);
    return output;
}

// ASCII特定的toupper.
template<class Char>
inline Char ToUpperASCII(Char c)
{
    return (c>='a' && c<='z') ? (c+('A'-'a')) : c;
}

template<class str>
inline void StringToUpperASCII(str* s)
{
    for(typename str::iterator i=s->begin(); i!=s->end(); ++i)
    {
        *i = ToUpperASCII(*i);
    }
}

template<class str>
inline str StringToUpperASCII(const str& s)
{
    // 针对std::string和std::wstring
    str output(s);
    StringToUpperASCII(&output);
    return output;
}

// 以小写方式比较给定的字符串. 校验输入字符串是否匹配一些标记时非常有用, 函数
// 做了优化避免生成临时字符拷贝.
bool LowerCaseEqualsASCII(const std::string& a, const char* b);
bool LowerCaseEqualsASCII(const std::wstring& a, const char* b);
bool LowerCaseEqualsASCII(const string16& a, const char* b);

// 使用迭代器.
bool LowerCaseEqualsASCII(std::string::const_iterator a_begin,
                          std::string::const_iterator a_end, const char* b);
bool LowerCaseEqualsASCII(std::wstring::const_iterator a_begin,
                          std::wstring::const_iterator a_end, const char* b);
bool LowerCaseEqualsASCII(string16::const_iterator a_begin,
                          string16::const_iterator a_end, const char* b);
bool LowerCaseEqualsASCII(const char* a_begin, const char* a_end, const char* b);
bool LowerCaseEqualsASCII(const wchar_t* a_begin, const wchar_t* a_end,
                          const char* b);
bool LowerCaseEqualsASCII(const char16* a_begin, const char16* a_end, const char* b);

// 大小写相关的字符串比较. 如果2个字符串都不是ASCII则行为不确定.
bool EqualsASCII(const string16& a, const base::StringPiece& b);

// 如果str以search开头返回true, 否则返回false.
bool StartsWithASCII(const std::string& str, const std::string& search,
                     bool case_sensitive);
bool StartsWith(const std::wstring& str, const std::wstring& search,
                bool case_sensitive);
bool StartsWith(const string16& str, const string16& search,
                bool case_sensitive);

// 如果str以search结尾返回true, 否则返回false.
bool EndsWith(const std::string& str, const std::string& search,
              bool case_sensitive);
bool EndsWith(const std::wstring& str, const std::wstring& search,
              bool case_sensitive);
bool EndsWith(const string16& str, const string16& search,
              bool case_sensitive);

// 确定ASCII字符类型.
template<typename Char>
inline bool IsAsciiWhitespace(Char c)
{
    return c==' ' || c=='\r' || c=='\n' || c=='\t';
}
template<typename Char>
inline bool IsAsciiAlpha(Char c)
{
    return ((c>='A') && (c<='Z')) || ((c>='a') && (c<='z'));
}
template<typename Char>
inline bool IsAsciiDigit(Char c)
{
    return c>='0' && c<='9';
}

template<typename Char>
inline bool IsHexDigit(Char c)
{
    return (c>='0' && c<='9') || (c>='A' && c<='F') || (c>='a' && c<='f');
}

template<typename Char>
inline Char HexDigitToInt(Char c)
{
    DCHECK(IsHexDigit(c));
    if(c>='0' && c<='9')
    {
        return c - '0';
    }
    if(c>='A' && c<='F')
    {
        return c - 'A' + 10;
    }
    if(c>='a' && c<='f')
    {
        return c - 'a' + 10;
    }
    return 0;
}

// 如果是空白字符则返回true.
inline bool IsWhitespace(wchar_t c)
{
    return wcschr(kWhitespaceWide, c) != NULL;
}

enum DataUnits
{
    DATA_UNITS_BYTE = 0,
    DATA_UNITS_KIBIBYTE,
    DATA_UNITS_MEBIBYTE,
    DATA_UNITS_GIBIBYTE,
};

// 返回字节的显示单位.
DataUnits GetByteDisplayUnits(int64 bytes);

// 返回可读的字节字符串, units指定了显示单位, 单位后缀可选.
// 例如: FormatBytes(512, DATA_UNITS_KIBIBYTE, true) => "0.5 KB"
// 例如: FormatBytes(100*1024, DATA_UNITS_MEBIBYTE, false) => "0.1"
string16 FormatBytes(int64 bytes, DataUnits units, bool show_units);

// 同上, 增加了"/s"单位.
// 例如: FormatSpeed(512, DATA_UNITS_KIBIBYTE, true) => "0.5 KB/s"
// 例如: FormatSpeed(100*1024, DATA_UNITS_MEBIBYTE, false) => "0.1"
string16 FormatSpeed(int64 bytes, DataUnits units, bool show_units);

// 返回数字的格式化.
// 例如: FormatNumber(1234567) => 1,234,567
string16 FormatNumber(int64 number);

// 从start_offset(通常为0)开始替换第一个|find_this|为|replace_with|.
void ReplaceFirstSubstringAfterOffset(string16* str,
                                      string16::size_type start_offset,
                                      const string16& find_this,
                                      const string16& replace_with);
void ReplaceFirstSubstringAfterOffset(std::string* str,
                                      std::string::size_type start_offset,
                                      const std::string& find_this,
                                      const std::string& replace_with);

// 从start_offset(通常为0)开始替换所有|find_this|为|replace_with|.
//
// 遍历整个子串; 对于单一字符使用<algorithm>中的std::replace, 例如:
//     std::replace(str.begin(), str.end(), 'a', 'b');
void ReplaceSubstringsAfterOffset(string16* str,
                                  string16::size_type start_offset,
                                  const string16& find_this,
                                  const string16& replace_with);
void ReplaceSubstringsAfterOffset(std::string* str,
                                  std::string::size_type start_offset,
                                  const std::string& find_this,
                                  const std::string& replace_with);

// 一种节省string拷贝的办法, 解决写数据wchar_t[]到std::wstring中的问题.
// 预留足够的空间并设置字符长度为正确的值保证.length()属性正常.
//
// reserve()分配空间需要在末尾多加一个空结束符. 这样做是因为resize()不能
// 保证预留空结束符. 调用resize()只是为了改变string的'length'成员.
//
// 性能: 调用wide.resize()是线性时间复杂度, 因为需要用空字符填充缓冲区.
// 这样做是因为需要改变字符串的实际长度(直接写缓冲区不会改变实际长度).
// 或许有常量时间复杂度改变string长度的办法.
template<class string_type>
inline typename string_type::value_type* WriteInto(string_type* str,
                                                   size_t length_with_null)
{
    str->reserve(length_with_null);
    str->resize(length_with_null-1);
    return &((*str)[0]);
}

// 辅助字符串比较/搜索的仿函数.

template<typename Char> struct CaseInsensitiveCompare
{
public:
    bool operator()(Char x, Char y) const
    {
        return tolower(x) == tolower(y);
    }
};

template<typename Char> struct CaseInsensitiveCompareASCII
{
public:
    bool operator()(Char x, Char y) const
    {
        return ToLowerASCII(x) == ToLowerASCII(y);
    }
};

// 使用|delimiters|任意字符切分字符串, 每一段都添加到|tokens|中,
// 返回找到的标记数量.
size_t Tokenize(const std::wstring& str, const std::wstring& delimiters,
                std::vector<std::wstring>* tokens);
size_t Tokenize(const string16& str, const string16& delimiters,
                std::vector<string16>* tokens);
size_t Tokenize(const std::string& str, const std::string& delimiters,
                std::vector<std::string>* tokens);
size_t Tokenize(const base::StringPiece& str,
                const base::StringPiece& delimiters,
                std::vector<base::StringPiece>* tokens);

// SplitString()的反操作.
string16 JoinString(const std::vector<string16>& parts, char16 s);
std::string JoinString(const std::vector<std::string>& parts, char s);

// 警告: 这里使用HTML5规范定义的空白. 用下面的函数可以实现去除字符串所有空白的
// 功能.
//
// 沿空白(HTML 5定义的五种空白字符)切分字符串. 连续的非空白字符块添加到结果中.
void SplitStringAlongWhitespace(const std::wstring& str,
                                std::vector<std::wstring>* result);
void SplitStringAlongWhitespace(const string16& str,
                                std::vector<string16>* result);
void SplitStringAlongWhitespace(const std::string& str,
                                std::vector<std::string>* result);

// 把$1-$2-$3..$9格式的字符串替换成|a|-|b|-|c|..|i|. 连续的'$'字符去掉一个,
// 例如: $$->$, $$$->$$等. offsets可以为NULL. 最多允许9次替换.
string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const std::vector<string16>& subst,
                                   std::vector<size_t>* offsets);

std::string ReplaceStringPlaceholders(const base::StringPiece& format_string,
                                      const std::vector<std::string>& subst,
                                      std::vector<size_t>* offsets);

// ReplaceStringPlaceholders的单一字符串简化版本. offsets可以为NULL.
string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const string16& a,
                                   size_t* offset);

// 如果|input|长度大于|max_len|, 函数返回true, 在保证最大长度的基础上使用...替换
// 中间的字符存于|output|.
// 例如: ElideString(L"Hello", 10, &str)输出Hello到output并返回false.
// ElideString(L"Hello my name is Tom", 10, &str)输出"Hell...Tom"到output并返回true.
bool ElideString(const std::wstring& input, int max_len, std::wstring* output);

// string匹配pattern成功返回true. pattern字符串可以包含通配符*和?.
// 反斜线字符(\)是*和?的转义字符. 限制pattern最多含有16个*或者?字符.
// ?匹配0或者1个字符, *匹配0或者多个字符.
bool MatchPattern(const base::StringPiece& string,
                  const base::StringPiece& pattern);
bool MatchPattern(const string16& string, const string16& pattern);

// 转换任意字符类型到对应的无符号类型.
// 例如: char, signed char, unsigned char -> unsigned char.
template<typename T>
struct ToUnsigned
{
    typedef T Unsigned;
};

template<>
struct ToUnsigned<char>
{
    typedef unsigned char Unsigned;
};
template<>
struct ToUnsigned<signed char>
{
    typedef unsigned char Unsigned;
};
template<>
struct ToUnsigned<wchar_t>
{
    typedef unsigned short Unsigned;
};
template<>
struct ToUnsigned<short>
{
    typedef unsigned short Unsigned;
};

#endif //__base_string_util_h__