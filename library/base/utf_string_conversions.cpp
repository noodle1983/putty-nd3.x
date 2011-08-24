
#include "utf_string_conversions.h"

#include "string_util.h"
#include "utf_string_conversion_utils.h"

using base::PrepareForUTF8Output;
using base::PrepareForUTF16Or32Output;
using base::ReadUnicodeCharacter;
using base::WriteUnicodeCharacter;

namespace
{

    // 转换源Unicode字符类型到目标Unicode字符类型的STL字符串.
    template<typename SRC_CHAR, typename DEST_STRING>
    bool ConvertUnicode(const SRC_CHAR* src,
        size_t src_len, DEST_STRING* output)
    {
        // ICU需要32位.
        bool success = true;
        int32 src_len32 = static_cast<int32>(src_len);
        for(int32 i=0; i<src_len32; i++)
        {
            uint32 code_point;
            if(ReadUnicodeCharacter(src, src_len32, &i, &code_point))
            {
                WriteUnicodeCharacter(code_point, output);
            }
            else
            {
                WriteUnicodeCharacter(0xFFFD, output);
                success = false;
            }
        }

        return success;
    }

}

// UTF-8 <-> Wide --------------------------------------------------------------

bool WideToUTF8(const wchar_t* src, size_t src_len, std::string* output)
{
    PrepareForUTF8Output(src, src_len, output);
    return ConvertUnicode(src, src_len, output);
}

std::string WideToUTF8(const std::wstring& wide)
{
    std::string ret;
    // 忽略调用返回值, 尽可能多的进行转换输出.
    WideToUTF8(wide.data(), wide.length(), &ret);
    return ret;
}

bool UTF8ToWide(const char* src, size_t src_len, std::wstring* output)
{
    PrepareForUTF16Or32Output(src, src_len, output);
    return ConvertUnicode(src, src_len, output);
}

std::wstring UTF8ToWide(const base::StringPiece& utf8)
{
    std::wstring ret;
    UTF8ToWide(utf8.data(), utf8.length(), &ret);
    return ret;
}

// UTF-16 <-> Wide -------------------------------------------------------------

// 当wide==UTF-16, 不用转换.
bool WideToUTF16(const wchar_t* src, size_t src_len, string16* output)
{
    output->assign(src, src_len);
    return true;
}

string16 WideToUTF16(const std::wstring& wide)
{
    return wide;
}

bool UTF16ToWide(const char16* src, size_t src_len, std::wstring* output)
{
    output->assign(src, src_len);
    return true;
}

std::wstring UTF16ToWide(const string16& utf16)
{
    return utf16;
}

// UTF16 <-> UTF8 --------------------------------------------------------------

bool UTF8ToUTF16(const char* src, size_t src_len, string16* output)
{
    return UTF8ToWide(src, src_len, output);
}

string16 UTF8ToUTF16(const std::string& utf8)
{
    return UTF8ToWide(utf8);
}

bool UTF16ToUTF8(const char16* src, size_t src_len, std::string* output)
{
    return WideToUTF8(src, src_len, output);
}

std::string UTF16ToUTF8(const string16& utf16)
{
    return WideToUTF8(utf16);
}

std::wstring ASCIIToWide(const base::StringPiece& ascii)
{
    DCHECK(IsStringASCII(ascii)) << ascii;
    return std::wstring(ascii.begin(), ascii.end());
}

string16 ASCIIToUTF16(const base::StringPiece& ascii)
{
    DCHECK(IsStringASCII(ascii)) << ascii;
    return string16(ascii.begin(), ascii.end());
}