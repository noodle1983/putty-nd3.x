
#ifndef __rfc_algorithm_json_writer_h__
#define __rfc_algorithm_json_writer_h__

#pragma once

#include <string>

#include "base/basic_types.h"

class Value;

namespace base
{

    class JSONWriter
    {
    public:
        // 根据给定的根节点生成JSON字符串并添加到|json|. 如果|pretty_print|为true,
        // 返回的格式化好的json字符串(用空格补白以便阅读). 如果|pretty_print|为
        // false, 生成尽可能紧凑的字符串.
        // TODO: 非法的情况下(比如|node|不是DictionaryValue/ListValue或者有正无穷
        // 大/负无穷大浮点数)是否应该生成json?
        static void Write(const Value* const node, bool pretty_print,
            std::string* json);

        // 同上. 有是否需要对字符串转义的选项, 用于以二进制方式(UTF8)传递结果串到
        // JSON解析器.
        static void WriteWithOptionalEscape(const Value* const node,
            bool pretty_print, bool escape, std::string* json);

        // JSON静态常量字符串, 表示空数组, 用于传递空的JSON参数.
        static const char* kEmptyArray;

    private:
        JSONWriter(bool pretty_print, std::string* json);

        // 递归调用构建JSON字符串. 完成时结果存于json_string_中.
        void BuildJSONString(const Value* const node, int depth, bool escape);

        // 添加一个用""括起来转义过的(UTF-8)字符串到json_string_.
        void AppendQuotedString(const std::string& str);

        // 添加空格到json_string_用于层级缩进.
        void IndentLine(int depth);

        // 存储生成的JSON数据.
        std::string* json_string_;

        bool pretty_print_;

        DISALLOW_COPY_AND_ASSIGN(JSONWriter);
    };

} //namespace base

#endif //__rfc_algorithm_json_writer_h__