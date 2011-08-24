
#include "string_util.h"

#include <algorithm>

#include "third_party/icu_base/icu_utf.h"

#include "memory/singleton.h"
#include "utf_string_conversions.h"
#include "utf_string_conversion_utils.h"

namespace
{
    // 强制Empty[W]String[16]使用的单件都是唯一类型, 防止其他代码不小心使用
    // Singleton<string>直接得到内部值.
    struct EmptyStrings
    {
        EmptyStrings() {}
        const std::string s;
        const std::wstring ws;
        const string16 s16;

        static EmptyStrings* GetInstance()
        {
            return Singleton<EmptyStrings>::get();
        }
    };

    // 被ReplaceStringPlaceholders用于跟踪替换参数在字符串中的位置.
    struct ReplacementOffset
    {
        ReplacementOffset(uintptr_t parameter, size_t offset)
            : parameter(parameter), offset(offset) {}

        // 参数索引.
        uintptr_t parameter;

        // 起始位置.
        size_t offset;
    };

    static bool CompareParameter(const ReplacementOffset& elem1,
        const ReplacementOffset& elem2)
    {
        return elem1.parameter < elem2.parameter;
    }
}

namespace base
{

    bool IsWprintfFormatPortable(const wchar_t* format)
    {
        for(const wchar_t* position=format; *position!='\0'; ++position)
        {
            if(*position == '%')
            {
                bool in_specification = true;
                bool modifier_l = false;
                while(in_specification)
                {
                    // 跳过所有字符直到遇到一个已知的格式符.
                    if(*++position == '\0')
                    {
                        // 格式控制在中间结束认为是可移植的, 因为没有发现不可移植的控制符.
                        return true;
                    }

                    if(*position == 'l')
                    {
                        // 只有'l'的后面能带's'和'c'控制符.
                        modifier_l = true;
                    }
                    else if(((*position=='s' || *position=='c') && !modifier_l) ||
                        *position=='S' || *position=='C' || *position=='F' ||
                        *position=='D' || *position=='O' || *position=='U')
                    {
                        // 不可移植
                        return false;
                    }

                    if(wcschr(L"diouxXeEfgGaAcspn%", *position))
                    {
                        // 可移植, 继续扫描剩余的格式串.
                        in_specification = false;
                    }
                }
            }
        }

        return true;
    }

} //namespace base

const std::string& EmptyString()
{
    return EmptyStrings::GetInstance()->s;
}

const std::wstring& EmptyWString()
{
    return EmptyStrings::GetInstance()->ws;
}

const string16& EmptyString16()
{
    return EmptyStrings::GetInstance()->s16;
}

#define WHITESPACE_UNICODE \
    0x0009, /* <control-0009> to <control-000D> */ \
    0x000A,                                        \
    0x000B,                                        \
    0x000C,                                        \
    0x000D,                                        \
    0x0020, /* Space */                            \
    0x0085, /* <control-0085> */                   \
    0x00A0, /* No-Break Space */                   \
    0x1680, /* Ogham Space Mark */                 \
    0x180E, /* Mongolian Vowel Separator */        \
    0x2000, /* En Quad to Hair Space */            \
    0x2001,                                        \
    0x2002,                                        \
    0x2003,                                        \
    0x2004,                                        \
    0x2005,                                        \
    0x2006,                                        \
    0x2007,                                        \
    0x2008,                                        \
    0x2009,                                        \
    0x200A,                                        \
    0x200C, /* Zero Width Non-Joiner */            \
    0x2028, /* Line Separator */                   \
    0x2029, /* Paragraph Separator */              \
    0x202F, /* Narrow No-Break Space */            \
    0x205F, /* Medium Mathematical Space */        \
    0x3000, /* Ideographic Space */                \
    0

const wchar_t kWhitespaceWide[] =
{
    WHITESPACE_UNICODE
};
const char16 kWhitespaceUTF16[] =
{
    WHITESPACE_UNICODE
};
const char kWhitespaceASCII[] =
{
    0x09, // <control-0009> to <control-000D>
    0x0A,
    0x0B,
    0x0C,
    0x0D,
    0x20, // Space
    0
};

const char kUtf8ByteOrderMark[] = "\xEF\xBB\xBF";

template<typename STR>
bool RemoveCharsT(const STR& input,
                  const typename STR::value_type remove_chars[],
                  STR* output)
{
    bool removed = false;
    size_t found;

    *output = input;

    found = output->find_first_of(remove_chars);
    while(found != STR::npos)
    {
        removed = true;
        output->replace(found, 1, STR());
        found = output->find_first_of(remove_chars, found);
    }

    return removed;
}

bool RemoveChars(const std::wstring& input,
                 const wchar_t remove_chars[],
                 std::wstring* output)
{
    return RemoveCharsT(input, remove_chars, output);
}

bool RemoveChars(const std::string& input,
                 const char remove_chars[],
                 std::string* output)
{
    return RemoveCharsT(input, remove_chars, output);
}

template<typename STR>
TrimPositions TrimStringT(const STR& input,
                          const typename STR::value_type trim_chars[],
                          TrimPositions positions,
                          STR* output)
{
    // 根据移除选项positions查找两端边界.
    const typename STR::size_type last_char = input.length() - 1;
    const typename STR::size_type first_good_char = (positions&TRIM_LEADING) ?
        input.find_first_not_of(trim_chars) : 0;
    const typename STR::size_type last_good_char = (positions&TRIM_TRAILING) ?
        input.find_last_not_of(trim_chars) : last_char;

    // 当字符串所有字符都是空白, 根据调用传入的positions返回TrimPositions.
    // 对于空输入没有去除任何空白, 但仍需要对output串调用clear.
    if(input.empty() || (first_good_char==STR::npos) || (last_good_char==STR::npos))
    {
        bool input_was_empty = input.empty();
        output->clear();
        return input_was_empty ? TRIM_NONE : positions;
    }

    // 移除空白.
    *output = input.substr(first_good_char, last_good_char-first_good_char+1);

    // 返回两端哪边移除过.
    return static_cast<TrimPositions>(
        ((first_good_char==0)?TRIM_NONE:TRIM_LEADING) |
        ((last_good_char==last_char)?TRIM_NONE:TRIM_TRAILING));
}

bool TrimString(const std::wstring& input,
                const wchar_t trim_chars[],
                std::wstring* output)
{
    return TrimStringT(input, trim_chars, TRIM_ALL, output) != TRIM_NONE;
}

bool TrimString(const std::string& input,
                const char trim_chars[],
                std::string* output)
{
    return TrimStringT(input, trim_chars, TRIM_ALL, output) != TRIM_NONE;
}

void TruncateUTF8ToByteSize(const std::string& input,
                            const size_t byte_size,
                            std::string* output)
{
    DCHECK(output);
    if(byte_size > input.length())
    {
        *output = input;
        return;
    }
    DCHECK_LE(byte_size, static_cast<uint32>(kint32max));
    // 注意: 由于CBU8_NEXT使用的是int32.
    int32 truncation_length = static_cast<int32>(byte_size);
    int32 char_index = truncation_length - 1;
    const char* data = input.data();

    // 使用CBU8, 从阶段点由后向前移动查找合法的UTF8字符. 一旦找到
    // 一个UTF8字符, 截断到该字符作为输出.
    while(char_index >= 0)
    {
        int32 prev = char_index;
        uint32 code_point = 0;
        CBU8_NEXT(data, char_index, truncation_length, code_point);
        if(!base::IsValidCharacter(code_point) ||
            !base::IsValidCodepoint(code_point))
        {
            char_index = prev - 1;
        }
        else
        {
            break;
        }
    }

    if(char_index >= 0)
    {
        *output = input.substr(0, char_index);
    }
    else
    {
        output->clear();
    }
}

TrimPositions TrimWhitespace(const std::wstring& input,
                             TrimPositions positions,
                             std::wstring* output)
{
    return TrimStringT(input, kWhitespaceWide, positions, output);
}

TrimPositions TrimWhitespace(const std::string& input,
                             TrimPositions positions,
                             std::string* output)
{
    return TrimWhitespaceASCII(input, positions, output);
}

TrimPositions TrimWhitespaceASCII(const std::string& input,
                                  TrimPositions positions,
                                  std::string* output)
{
    return TrimStringT(input, kWhitespaceASCII, positions, output);
}

template<typename STR>
STR CollapseWhitespaceT(const STR& text,
                        bool trim_sequences_with_line_breaks)
{
    STR result;
    result.resize(text.size());

    // 设置标志位为true假设已经在连续空白中, 这样可以移除开头的全部空白.
    bool in_whitespace = true;
    bool already_trimmed = true;

    int chars_written = 0;
    for(typename STR::const_iterator i=text.begin(); i!=text.end(); ++i)
    {
        if(IsWhitespace(*i))
        {
            if(!in_whitespace)
            {
                // 减少连续空白至一个空白.
                in_whitespace = true;
                result[chars_written++] = L' ';
            }
            if(trim_sequences_with_line_breaks && !already_trimmed &&
                ((*i=='\n') || (*i=='\r')))
            {
                // 包含回车换行的空白序列全部移除.
                already_trimmed = true;
                --chars_written;
            }
        }
        else
        {
            // 非空白字符直接拷贝.
            in_whitespace = false;
            already_trimmed = false;
            result[chars_written++] = *i;
        }
    }

    if(in_whitespace && !already_trimmed)
    {
        // 忽略末尾的全部空白.
        --chars_written;
    }

    result.resize(chars_written);
    return result;
}

std::wstring CollapseWhitespace(const std::wstring& text,
                                bool trim_sequences_with_line_breaks)
{
    return CollapseWhitespaceT(text, trim_sequences_with_line_breaks);
}

std::string CollapseWhitespaceASCII(const std::string& text,
                                    bool trim_sequences_with_line_breaks)
{
    return CollapseWhitespaceT(text, trim_sequences_with_line_breaks);
}

bool ContainsOnlyWhitespaceASCII(const std::string& str)
{
    for(std::string::const_iterator i=str.begin(); i!=str.end(); ++i)
    {
        if(!IsAsciiWhitespace(*i))
        {
            return false;
        }
    }
    return true;
}

bool ContainsOnlyWhitespace(const string16& str)
{
    for(string16::const_iterator i=str.begin(); i!=str.end(); ++i)
    {
        if(!IsWhitespace(*i))
        {
            return false;
        }
    }
    return true;
}

template<typename STR>
static bool ContainsOnlyCharsT(const STR& input, const STR& characters)
{
    for(typename STR::const_iterator iter=input.begin();
        iter!=input.end(); ++iter)
    {
        if(characters.find(*iter) == STR::npos)
        {
            return false;
        }
    }
    return true;
}

bool ContainsOnlyChars(const std::wstring& input,
                       const std::wstring& characters)
{
    return ContainsOnlyCharsT(input, characters);
}

bool ContainsOnlyChars(const std::string& input,
                       const std::string& characters)
{
    return ContainsOnlyCharsT(input, characters);
}

std::string WideToASCII(const std::wstring& wide)
{
    DCHECK(IsStringASCII(wide)) << wide;
    return std::string(wide.begin(), wide.end());
}

std::string UTF16ToASCII(const string16& utf16)
{
    DCHECK(IsStringASCII(utf16)) << utf16;
    return std::string(utf16.begin(), utf16.end());
}

// Latin1是Unicode的低位字节, 所以可以直接赋值转换.
bool WideToLatin1(const std::wstring& wide, std::string* latin1)
{
    std::string output;
    output.resize(wide.size());
    latin1->clear();
    for(size_t i=0; i<wide.size(); ++i)
    {
        if(wide[i] > 255)
        {
            return false;
        }
        output[i] = static_cast<char>(wide[i]);
    }
    latin1->swap(output);
    return true;
}

template<class STR>
static bool DoIsStringASCII(const STR& str)
{
    for(size_t i=0; i<str.length(); ++i)
    {
        typename ToUnsigned<typename STR::value_type>::Unsigned c = str[i];
        if(c > 0x7F)
        {
            return false;
        }
    }
    return true;
}

bool IsStringASCII(const std::wstring& str)
{
    return DoIsStringASCII(str);
}

bool IsStringASCII(const base::StringPiece& str)
{
    return DoIsStringASCII(str);
}

bool IsStringUTF8(const std::string& str)
{
    const char* src = str.data();
    int32 src_len = static_cast<int32>(str.length());
    int32 char_index = 0;

    while(char_index < src_len)
    {
        int32 code_point;
        CBU8_NEXT(src, char_index, src_len, code_point);
        if(!base::IsValidCharacter(code_point))
        {
            return false;
        }
    }
    return true;
}

template<typename Iter>
static inline bool DoLowerCaseEqualsASCII(Iter a_begin,
                                          Iter a_end,
                                          const char* b)
{
    for(Iter it=a_begin; it!=a_end; ++it,++b)
    {
        if(!*b || ToLowerASCII(*it)!=*b)
        {
            return false;
        }
    }
    return *b == 0;
}

bool LowerCaseEqualsASCII(const std::string& a, const char* b)
{
    return DoLowerCaseEqualsASCII(a.begin(), a.end(), b);
}

bool LowerCaseEqualsASCII(const std::wstring& a, const char* b)
{
    return DoLowerCaseEqualsASCII(a.begin(), a.end(), b);
}

bool LowerCaseEqualsASCII(std::string::const_iterator a_begin,
                          std::string::const_iterator a_end,
                          const char* b)
{
    return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}

bool LowerCaseEqualsASCII(std::wstring::const_iterator a_begin,
                          std::wstring::const_iterator a_end,
                          const char* b)
{
    return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}

bool LowerCaseEqualsASCII(const char* a_begin,
                          const char* a_end,
                          const char* b)
{
    return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}

bool LowerCaseEqualsASCII(const wchar_t* a_begin,
                          const wchar_t* a_end,
                          const char* b)
{
    return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}

bool EqualsASCII(const string16& a, const base::StringPiece& b)
{
    if(a.length() != b.length())
    {
        return false;
    }
    return std::equal(b.begin(), b.end(), a.begin());
}

bool StartsWithASCII(const std::string& str,
                     const std::string& search,
                     bool case_sensitive)
{
    if(case_sensitive)
    {
        return str.compare(0, search.length(), search) == 0;
    }
    else
    {
        return base::strncasecmp(str.c_str(), search.c_str(), search.length()) == 0;
    }
}

template <typename STR>
bool StartsWithT(const STR& str, const STR& search, bool case_sensitive)
{
    if(case_sensitive)
    {
        return str.compare(0, search.length(), search) == 0;
    }
    else
    {
        if(search.size() > str.size())
        {
            return false;
        }
        return std::equal(search.begin(), search.end(), str.begin(),
            CaseInsensitiveCompare<typename STR::value_type>());
    }
}

bool StartsWith(const std::wstring& str, const std::wstring& search,
                bool case_sensitive)
{
    return StartsWithT(str, search, case_sensitive);
}

template <typename STR>
bool EndsWithT(const STR& str, const STR& search, bool case_sensitive)
{
    typename STR::size_type str_length = str.length();
    typename STR::size_type search_length = search.length();
    if(search_length > str_length)
    {
        return false;
    }
    if(case_sensitive)
    {
        return str.compare(str_length - search_length, search_length, search) == 0;
    }
    else
    {
        return std::equal(search.begin(), search.end(),
            str.begin()+(str_length-search_length),
            CaseInsensitiveCompare<typename STR::value_type>());
    }
}

bool EndsWith(const std::string& str, const std::string& search,
              bool case_sensitive)
{
    return EndsWithT(str, search, case_sensitive);
}

bool EndsWith(const std::wstring& str, const std::wstring& search,
              bool case_sensitive)
{
    return EndsWithT(str, search, case_sensitive);
}

static const char* const kByteStringsUnlocalized[] =
{
    " B",
    " kB",
    " MB",
    " GB",
    " TB",
    " PB"
};

string16 FormatBytesUnlocalized(int64 bytes)
{
    double unit_amount = static_cast<double>(bytes);
    size_t dimension = 0;
    const int kKilo = 1024;
    while(unit_amount>=kKilo &&
        dimension<arraysize(kByteStringsUnlocalized)-1)
    {
        unit_amount /= kKilo;
        dimension++;
    }

    char buf[64];
    if(bytes!=0 && dimension>0 && unit_amount<100)
    {
        base::snprintf(buf, arraysize(buf), "%.1lf%s", unit_amount,
            kByteStringsUnlocalized[dimension]);
    }
    else
    {
        base::snprintf(buf, arraysize(buf), "%.0lf%s", unit_amount,
            kByteStringsUnlocalized[dimension]);
    }

    return ASCIIToUTF16(buf);
}

template<class StringType>
void DoReplaceSubstringsAfterOffset(StringType* str,
                                    typename StringType::size_type start_offset,
                                    const StringType& find_this,
                                    const StringType& replace_with,
                                    bool replace_all)
{
    if((start_offset==StringType::npos) || (start_offset>=str->length()))
    {
        return;
    }

    DCHECK(!find_this.empty());
    for(typename StringType::size_type offs(str->find(find_this, start_offset));
        offs!=StringType::npos; offs=str->find(find_this, offs))
    {
        str->replace(offs, find_this.length(), replace_with);
        offs += replace_with.length();

        if(!replace_all)
        {
            break;
        }
    }
}

void ReplaceFirstSubstringAfterOffset(string16* str,
                                      string16::size_type start_offset,
                                      const string16& find_this,
                                      const string16& replace_with)
{
    DoReplaceSubstringsAfterOffset(str, start_offset, find_this, replace_with,
        false); // 首次替换
}

void ReplaceFirstSubstringAfterOffset(std::string* str,
                                      std::string::size_type start_offset,
                                      const std::string& find_this,
                                      const std::string& replace_with)
{
    DoReplaceSubstringsAfterOffset(str, start_offset, find_this, replace_with,
        false); // 首次替换
}

void ReplaceSubstringsAfterOffset(string16* str,
                                  string16::size_type start_offset,
                                  const string16& find_this,
                                  const string16& replace_with)
{
    DoReplaceSubstringsAfterOffset(str, start_offset, find_this, replace_with,
        true); // 全部替换
}

void ReplaceSubstringsAfterOffset(std::string* str,
                                  std::string::size_type start_offset,
                                  const std::string& find_this,
                                  const std::string& replace_with)
{
    DoReplaceSubstringsAfterOffset(str, start_offset, find_this, replace_with,
        true); // 全部替换
}

template<typename STR>
static size_t TokenizeT(const STR& str,
                        const STR& delimiters,
                        std::vector<STR>* tokens)
{
    tokens->clear();

    typename STR::size_type start = str.find_first_not_of(delimiters);
    while(start != STR::npos)
    {
        typename STR::size_type end = str.find_first_of(delimiters, start+1);
        if(end == STR::npos)
        {
            tokens->push_back(str.substr(start));
            break;
        }
        else
        {
            tokens->push_back(str.substr(start, end-start));
            start = str.find_first_not_of(delimiters, end+1);
        }
    }

    return tokens->size();
}

size_t Tokenize(const std::wstring& str,
                const std::wstring& delimiters,
                std::vector<std::wstring>* tokens)
{
    return TokenizeT(str, delimiters, tokens);
}

size_t Tokenize(const std::string& str,
                const std::string& delimiters,
                std::vector<std::string>* tokens)
{
    return TokenizeT(str, delimiters, tokens);
}

size_t Tokenize(const base::StringPiece& str,
                const base::StringPiece& delimiters,
                std::vector<base::StringPiece>* tokens)
{
    return TokenizeT(str, delimiters, tokens);
}

template<typename STR>
static STR JoinStringT(const std::vector<STR>& parts,
                       typename STR::value_type sep)
{
    if(parts.size() == 0) return STR();

    STR result(parts[0]);
    typename std::vector<STR>::const_iterator iter = parts.begin();
    ++iter;

    for(; iter!=parts.end(); ++iter)
    {
        result += sep;
        result += *iter;
    }

    return result;
}

std::string JoinString(const std::vector<std::string>& parts, char sep)
{
    return JoinStringT(parts, sep);
}

string16 JoinString(const std::vector<string16>& parts, char16 sep)
{
    return JoinStringT(parts, sep);
}

template<typename STR>
void SplitStringAlongWhitespaceT(const STR& str, std::vector<STR>* result)
{
    const size_t length = str.length();
    if(!length)
    {
        return;
    }

    bool last_was_ws = false;
    size_t last_non_ws_start = 0;
    for(size_t i=0; i<length; ++i)
    {
        switch(str[i])
        {
            // HTML 5定义的空白: space, tab, LF, line tab, FF, or CR.
        case L' ':
        case L'\t':
        case L'\xA':
        case L'\xB':
        case L'\xC':
        case L'\xD':
            if(!last_was_ws)
            {
                if(i > 0)
                {
                    result->push_back(str.substr(last_non_ws_start,
                        i-last_non_ws_start));
                }
                last_was_ws = true;
            }
            break;

        default: // 不是空白字符.
            if(last_was_ws)
            {
                last_was_ws = false;
                last_non_ws_start = i;
            }
            break;
        }
    }
    if(!last_was_ws)
    {
        result->push_back(str.substr(last_non_ws_start,
            length-last_non_ws_start));
    }
}

void SplitStringAlongWhitespace(const std::wstring& str,
                                std::vector<std::wstring>* result)
{
    SplitStringAlongWhitespaceT(str, result);
}

void SplitStringAlongWhitespace(const std::string& str,
                                std::vector<std::string>* result)
{
    SplitStringAlongWhitespaceT(str, result);
}

template<class FormatStringType, class OutStringType>
OutStringType DoReplaceStringPlaceholders(const FormatStringType& format_string,
                                          const std::vector<OutStringType>& subst,
                                          std::vector<size_t>* offsets)
{
    size_t substitutions = subst.size();
    DCHECK(substitutions < 10);

    size_t sub_length = 0;
    for(typename std::vector<OutStringType>::const_iterator iter=subst.begin();
        iter!=subst.end(); ++iter)
    {
        sub_length += iter->length();
    }

    OutStringType formatted;
    formatted.reserve(format_string.length()+sub_length);

    std::vector<ReplacementOffset> r_offsets;
    for(typename FormatStringType::const_iterator i=format_string.begin();
        i!=format_string.end(); ++i)
    {
        if('$' == *i)
        {
            if(i+1 != format_string.end())
            {
                ++i;
                DCHECK('$'==*i || '1'<=*i) << "Invalid placeholder: " << *i;
                if('$' == *i)
                {
                    while(i!=format_string.end() && '$'==*i)
                    {
                        formatted.push_back('$');
                        ++i;
                    }
                    --i;
                }
                else
                {
                    uintptr_t index = *i - '1';
                    if(offsets)
                    {
                        ReplacementOffset r_offset(index,
                            static_cast<int>(formatted.size()));
                        r_offsets.insert(std::lower_bound(r_offsets.begin(),
                            r_offsets.end(),
                            r_offset,
                            &CompareParameter),
                            r_offset);
                    }
                    if(index < substitutions)
                    {
                        formatted.append(subst.at(index));
                    }
                }
            }
        }
        else
        {
            formatted.push_back(*i);
        }
    }
    if(offsets)
    {
        for(std::vector<ReplacementOffset>::const_iterator i=r_offsets.begin();
            i!=r_offsets.end(); ++i)
        {
            offsets->push_back(i->offset);
        }
    }
    return formatted;
}

string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const std::vector<string16>& subst,
                                   std::vector<size_t>* offsets)
{
    return DoReplaceStringPlaceholders(format_string, subst, offsets);
}

std::string ReplaceStringPlaceholders(const base::StringPiece& format_string,
                                      const std::vector<std::string>& subst,
                                      std::vector<size_t>* offsets)
{
    return DoReplaceStringPlaceholders(format_string, subst, offsets);
}

string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const string16& a,
                                   size_t* offset)
{
    std::vector<size_t> offsets;
    std::vector<string16> subst;
    subst.push_back(a);
    string16 result = ReplaceStringPlaceholders(format_string, subst, &offsets);

    DCHECK(offsets.size() == 1);
    if(offset)
    {
        *offset = offsets[0];
    }
    return result;
}

static bool IsWildcard(base_icu::UChar32 character)
{
    return character=='*' || character=='?';
}

// 移动strings指针到不匹配的起始点.
template<typename CHAR, typename NEXT>
static void EatSameChars(const CHAR** pattern, const CHAR* pattern_end,
                         const CHAR** string, const CHAR* string_end,
                         NEXT next)
{
    const CHAR* escape = NULL;
    while(*pattern!=pattern_end && *string!=string_end)
    {
        if(!escape && IsWildcard(**pattern))
        {
            // 遇到没有转义的通配符直接返回.
            return;
        }

        // 跳过转义字符移动到下一个字符.
        if(!escape && **pattern=='\\')
        {
            escape = *pattern;
            next(pattern, pattern_end);
            continue;
        }

        // 字符匹配则递增指针.
        const CHAR* pattern_next = *pattern;
        const CHAR* string_next = *string;
        base_icu::UChar32 pattern_char = next(&pattern_next, pattern_end);
        if(pattern_char==next(&string_next, string_end) &&
            pattern_char!=(base_icu::UChar32)CBU_SENTINEL)
        {
            *pattern = pattern_next;
            *string = string_next;
        }
        else
        {
            // 不匹配的时候函数返回. 如果上一个是转义字符, 意味着向前比较时发生错误,
            // 需要后退一个字符. 因为不能匹配转义字符, MatchPattern函数将返回false.
            if(escape)
            {
                *pattern = escape;
            }
            return;
        }

        escape = NULL;
    }
}

template<typename CHAR, typename NEXT>
static void EatWildcard(const CHAR** pattern, const CHAR* end, NEXT next)
{
    while(*pattern != end)
    {
        if(!IsWildcard(**pattern))
        {
            return;
        }
        next(pattern, end);
    }
}

template<typename CHAR, typename NEXT>
static bool MatchPatternT(const CHAR* eval, const CHAR* eval_end,
                          const CHAR* pattern, const CHAR* pattern_end,
                          int depth, NEXT next)
{
    const int kMaxDepth = 16;
    if(depth > kMaxDepth)
    {
        return false;
    }

    // 跳过所有匹配的字符.
    EatSameChars(&pattern, pattern_end, &eval, eval_end, next);

    // 字符串为空时模式串为空或者仅含有通配符.
    if(eval == eval_end)
    {
        EatWildcard(&pattern, pattern_end, next);
        return pattern == pattern_end;
    }

    // 模式串为空而字符串不为空, 匹配失败.
    if(pattern == pattern_end)
    {
        return false;
    }

    // 如果是?, 拿当前模式串和剩余字符串或者跳过一个字符的剩余串进行比较.
    const CHAR* next_pattern = pattern;
    next(&next_pattern, pattern_end);
    if(pattern[0] == '?')
    {
        if(MatchPatternT(eval, eval_end, next_pattern, pattern_end,
            depth+1, next))
        {
            return true;
        }
        const CHAR* next_eval = eval;
        next(&next_eval, eval_end);
        if(MatchPatternT(next_eval, eval_end, next_pattern, pattern_end,
            depth+1, next))
        {
            return true;
        }
    }

    // 如果是*, 尝试所有可能的子串和剩余的模式串进行匹配.
    if(pattern[0] == '*')
    {
        // 跳过重复的通配符(********** into *)以减少不必要的循环.
        EatWildcard(&next_pattern, pattern_end, next);

        while(eval != eval_end)
        {
            if(MatchPatternT(eval, eval_end, next_pattern, pattern_end,
                depth+1, next))
            {
                return true;
            }
            eval++;
        }

        // 到达字符串末尾, 检查模式串是否只含有通配符.
        if(eval == eval_end)
        {
            EatWildcard(&pattern, pattern_end, next);
            if(pattern != pattern_end)
            {
                return false;
            }
            return true;
        }
    }

    return false;
}

struct NextCharUTF8
{
    base_icu::UChar32 operator()(const char** p, const char* end)
    {
        base_icu::UChar32 c;
        int offset = 0;
        CBU8_NEXT(*p, offset, end-*p, c);
        *p += offset;
        return c;
    }
};

struct NextCharUTF16
{
    base_icu::UChar32 operator()(const char16** p, const char16* end)
    {
        base_icu::UChar32 c;
        int offset = 0;
        CBU16_NEXT(*p, offset, end-*p, c);
        *p += offset;
        return c;
    }
};

bool MatchPattern(const base::StringPiece& eval,
                  const base::StringPiece& pattern)
{
    return MatchPatternT(eval.data(), eval.data()+eval.size(),
        pattern.data(), pattern.data()+pattern.size(),
        0, NextCharUTF8());
}

bool MatchPattern(const string16& eval, const string16& pattern)
{
    return MatchPatternT(eval.c_str(), eval.c_str()+eval.size(),
        pattern.c_str(), pattern.c_str()+pattern.size(),
        0, NextCharUTF16());
}

// 以下代码和OpenBSD的lcpy接口兼容. 参见:
//   http://www.gratisoft.us/todd/papers/strlcpy.html
//   ftp://ftp.openbsd.org/pub/OpenBSD/src/lib/libc/string/{wcs,str}lcpy.c

namespace
{

    template<typename CHAR>
    size_t lcpyT(CHAR* dst, const CHAR* src, size_t dst_size)
    {
        for(size_t i=0; i<dst_size; ++i)
        {
            if((dst[i] = src[i]) == 0) // 遇到NULL拷贝并返回.
            {
                return i;
            }
        }

        // 保证Null结尾.
        if(dst_size != 0)
        {
            dst[dst_size-1] = 0;
        }

        // 计数|src|剩余的长度, 返回总长度.
        while(src[dst_size]) ++dst_size;
        return dst_size;
    }

}

size_t base::strlcpy(char* dst, const char* src, size_t dst_size)
{
    return lcpyT<char>(dst, src, dst_size);
}

size_t base::wcslcpy(wchar_t* dst, const wchar_t* src, size_t dst_size)
{
    return lcpyT<wchar_t>(dst, src, dst_size);
}