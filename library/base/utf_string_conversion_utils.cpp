
#include "utf_string_conversion_utils.h"

#include "third_party/icu_base/icu_utf.h"

namespace base
{

    bool ReadUnicodeCharacter(const char* src, int32 src_len,
        int32* char_index, uint32* code_point)
    {
        // U8_NEXT使用-1表示错误, 因此code_point使用有符号类型.
        // 函数出错时返回false, 因此code_point使用无符号类型.
        int32 cp;
        CBU8_NEXT(src, *char_index, src_len, cp);
        *code_point = static_cast<uint32>(cp);

        // 上面的ICU宏移到下一字符, 函数要求移到最后一个用掉的字符.
        (*char_index)--;

        // 验证解码值的合法性 .
        return IsValidCodepoint(cp);
    }

    bool ReadUnicodeCharacter(const char16* src, int32 src_len,
        int32* char_index, uint32* code_point)
    {
        if(CBU16_IS_SURROGATE(src[*char_index]))
        {
            if(!CBU16_IS_SURROGATE_LEAD(src[*char_index]) ||
                *char_index+1>=src_len ||
                !CBU16_IS_TRAIL(src[*char_index+1]))
            {
                // 非法的高代理对.
                return false;
            }

            // 合法的高代理对.
            *code_point = CBU16_GET_SUPPLEMENTARY(src[*char_index],
                src[*char_index+1]);
            (*char_index)++;
        }
        else
        {
            // 不是高代理, 16-bit字长.
            *code_point = src[*char_index];
        }

        return IsValidCodepoint(*code_point);
    }

    size_t WriteUnicodeCharacter(uint32 code_point, std::string* output)
    {
        if(code_point <= 0x7f)
        {
            // 常见的一字节情况.
            output->push_back(code_point);
            return 1;
        }

        // CBU8_APPEND_UNSAFE可以附加4字节.
        size_t char_offset = output->length();
        size_t original_char_offset = char_offset;
        output->resize(char_offset+CBU8_MAX_LENGTH);

        CBU8_APPEND_UNSAFE(&(*output)[0], char_offset, code_point);

        // CBU8_APPEND_UNSAFE会移动指针到插入字符后, 所以能知道字符串长度.
        output->resize(char_offset);
        return char_offset - original_char_offset;
    }

    size_t WriteUnicodeCharacter(uint32 code_point, string16* output)
    {
        if(CBU16_LENGTH(code_point) == 1)
        {
            // 码点在基本多文种平面.
            output->push_back(static_cast<char16>(code_point));
            return 1;
        }
        // 非基本多文种平面字符使用双字节编码.
        size_t char_offset = output->length();
        output->resize(char_offset+CBU16_MAX_LENGTH);
        CBU16_APPEND_UNSAFE(&(*output)[0], char_offset, code_point);
        return CBU16_MAX_LENGTH;
    }

    template<typename CHAR>
    void PrepareForUTF8Output(const CHAR* src,
        size_t src_len, std::string* output)
    {
        output->clear();
        if(src_len == 0)
        {
            return;
        }
        if(src[0] < 0x80)
        {
            // 假定输入全部是ASCII.
            output->reserve(src_len);
        }
        else
        {
            // 假定输入全部是非非ASCII且都是三字节编码.
            output->reserve(src_len*3);
        }
    }

    // 具现已知的调用版本.
    template void PrepareForUTF8Output(const wchar_t*, size_t, std::string*);
    template void PrepareForUTF8Output(const char16*, size_t, std::string*);

    template<typename STRING>
    void PrepareForUTF16Or32Output(const char* src,
        size_t src_len, STRING* output)
    {
        output->clear();
        if(src_len == 0)
        {
            return;
        }
        if(static_cast<unsigned char>(src[0]) < 0x80)
        {
            // 假定输入全部是ASCII.
            output->reserve(src_len);
        }
        else
        {
            // Otherwise assume that the UTF-8 sequences will have 2 bytes for each
            // character.
            // 假定输入的UTF-8序列每2字节表示一个字符.
            output->reserve(src_len/2);
        }
    }

    // 具现已知的调用版本.
    template void PrepareForUTF16Or32Output(const char*, size_t, std::wstring*);
    template void PrepareForUTF16Or32Output(const char*, size_t, string16*);

} //namespace base