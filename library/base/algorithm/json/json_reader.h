
#ifndef __base_json_reader_h__
#define __base_json_reader_h__

#pragma once

#include <string>

#include "base/basic_types.h"

// JSON解析器. 转换JSON成Value对象.
// http://www.ietf.org/rfc/rfc4627.txt?number=4627
//
//
// 与RFC的出入:
// - 只能解析带符号的32位整数和带小数的double.
// - 要求输入的编码是UTF8, 规范允许使用UTF-16(BE或者LE)或者UTF-32(BE或者LE)
//   编码.
// - 限制最多嵌套100层以防栈溢出(规范没有限制).
// - Unicode常见问题就是会在数据流前面写入Unicode Byte-Order-Mark (U+FEFF),
//   比如传递给JSONReader::JsonToValue()函数的UTF-8字符串的开头有可能是
//   UTF-8 BOM (0xEF, 0xBB, 0xBF). 为了避免把UTF-8 BOM错误的处理成合法字符,
//   函数在解析前跳过Unicode字符串开头的Unicode BOM.
//
// TODO: 添加解析选项控制keys是否必须用""括起来.
// TODO: 添加选项控制是否跳过注释.

namespace base
{

    class Value;

    class JSONReader
    {
    public:
        // 保存JS token.
        class Token
        {
        public:
            enum Type
            {
                OBJECT_BEGIN,           // {
                OBJECT_END,             // }
                ARRAY_BEGIN,            // [
                ARRAY_END,              // ]
                STRING,
                NUMBER,
                BOOL_TRUE,              // true
                BOOL_FALSE,             // false
                NULL_TOKEN,             // null
                LIST_SEPARATOR,         // ,
                OBJECT_PAIR_SEPARATOR,  // :
                END_OF_INPUT,
                INVALID_TOKEN,
            };

            Token(Type t, const wchar_t* b, int len)
                : type(t), begin(b), length(len) {}

            // 返回token结束位置后的字符.
            wchar_t NextChar()
            {
                return *(begin + length);
            }

            static Token CreateInvalidToken()
            {
                return Token(INVALID_TOKEN, 0, 0);
            }

            Type type;

            // 指向token在JSONReader::json_pos_中始位置的指针.
            const wchar_t* begin;

            // 结束位置是token后的一个字符.
            int length;
        };

        // 解析过程中的错误码.
        enum JsonParseError
        {
            JSON_NO_ERROR = 0,
            JSON_BAD_ROOT_ELEMENT_TYPE,
            JSON_INVALID_ESCAPE,
            JSON_SYNTAX_ERROR,
            JSON_TRAILING_COMMA,
            JSON_TOO_MUCH_NESTING,
            JSON_UNEXPECTED_DATA_AFTER_ROOT,
            JSON_UNSUPPORTED_ENCODING,
            JSON_UNQUOTED_DICTIONARY_KEY,
        };

        // 错误码对应的字符串.
        static const char* kBadRootElementType;
        static const char* kInvalidEscape;
        static const char* kSyntaxError;
        static const char* kTrailingComma;
        static const char* kTooMuchNesting;
        static const char* kUnexpectedDataAfterRoot;
        static const char* kUnsupportedEncoding;
        static const char* kUnquotedDictionaryKey;

        JSONReader();

        // 读取并解析|json|, 返回Value对象, 调用者拥有返回对象的所有权. 如果|json|
        // 格式有误, 返回NULL. 如果|allow_trailing_comma|为true, 忽略对象和数组结尾
        // 的逗号, 这种做法有悖于RFC.
        static Value* Read(const std::string& json, bool allow_trailing_comma);

        // 读取并解析|json|. |error_code_out|和|error_msg_out|是可选的, 如果指定了,
        // 函数返回NULL的时候, 会将错误码和错误消息(最好包括错误位置)存储其中, 否则
        // 不会做任何修改.
        static Value* ReadAndReturnError(const std::string& json,
            bool allow_trailing_comma,
            int* error_code_out,
            std::string* error_msg_out);

        // 转换JSON错误码成可读的消息. 如果error_code是JSON_NO_ERROR返回空字符串.
        static std::string ErrorCodeToString(JsonParseError error_code);

        // 调用JsonToValue()失败时, 返回错误码, 否则返回JSON_NO_ERROR.
        JsonParseError error_code() const { return error_code_; }

        // 转换JSON错误码成可读的消息, 最好能带上行列号.
        std::string GetErrorMessage() const;

        // 读取并解析|json|, 返回Value对象, 调用者拥有返回对象的所有权. 如果|json|
        // 格式有误, 返回NULL, 可以通过|error_message()|获取详细的错误信息. 如果
        // |allow_trailing_comma|为true, 忽略对象和数组结尾的逗号, 这种做法有悖
        // 于RFC.
        Value* JsonToValue(const std::string& json, bool check_root,
            bool allow_trailing_comma);

    private:
        static std::string FormatErrorMessage(int line, int column,
            const std::string& description);

        // 递归构建Value. 如果JSON字符串不合法则返回NULL. 如果|is_root|为true, 验证
        // 根元素是否为对象或者数组.
        Value* BuildValue(bool is_root);

        // 把字符序列解析成Token::NUMBER. 如果序列中的字符串不是合法的数字, 返回
        // Token::INVALID_TOKEN. 实际内部调用的是DecodeNumber, 此函数把字符串转换
        // 成int/double.
        Token ParseNumberToken();

        // 转换token子串成int或者double. 如果可以转换(比如不溢出)返回Value, 否则
        // 返回NULL.
        Value* DecodeNumber(const Token& token);

        // 把字符序列解析成Token::STRING. 如果序列中的字符串不是合法的数字, 返回
        // Token::INVALID_TOKEN. 实际内部调用的是DecodeString, 此函数把字符串转换
        // 成wstring.
        Token ParseStringToken();

        // 转换子串成合法的Value字符串. 函数总是成功(否则ParseStringToken会失败).
        Value* DecodeString(const Token& token);

        // 获取JSON流中下一个token. 不修改当前位置, 这样临时读取后面的token.
        Token ParseToken();

        // 向前移动|json_pos_|跳过空白和注释.
        void EatWhitespaceAndComments();

        // 如果|json_pos_|在注释起始位置则跳过注释, 否则返回false.
        bool EatComment();

        // 检查|json_pos_|是否匹配str.
        bool NextStringMatch(const wchar_t* str, size_t length);

        // 设置返回给调用者的错误码. 确定行列号并添加到error_pos中.
        void SetErrorCode(const JsonParseError error, const wchar_t* error_pos);

        // 指向输入字符串起始位置的指针.
        const wchar_t* start_pos_;

        // 指向输入字符串当前位置的指针.
        const wchar_t* json_pos_;

        // 维护lists/dicts嵌套层数.
        int stack_depth_;

        // 对象和数组结尾是否允许有逗号的解析选项.
        bool allow_trailing_comma_;

        // 最近一次调用JsonToValue()的错误码.
        JsonParseError error_code_;
        int error_line_;
        int error_col_;

        DISALLOW_COPY_AND_ASSIGN(JSONReader);
    };

} //namespace base

#endif //__base_json_reader_h__