
#ifndef __base_vlog_h__
#define __base_vlog_h__

#pragma once

#include <vector>

#include "basic_types.h"
#include "string_piece.h"

namespace base
{

    // 包含日志记录所有设置的辅助类.
    class VlogInfo
    {
    public:
        static const int kDefaultVlogLevel;

        // |v_switch|指定缺省的最大激活日志记录等级, 默认是0, 一般使用正数.
        //
        // |vmodule_switch|给出每个模块的最大激活日志记录等级, 覆盖|v_switch|
        // 提供的值.
        // 例如: "my_module=2,foo*=3"将改变所有"my_module.*"和"foo*.*"(
        // 匹配时"-inl"后缀会被预先忽略掉)源文件中代码的日志等级.
        //
        // |min_log_level|指向int指针存储日志记录等级. 如果解析到合法的
        // |v_switch|, 会设置该值, 缺省的日志记录等级从这里读取.
        //
        // 含有\或者/的模式会匹配整个路径而不是一个模块. 例如
        // "*/foo/bar/*=2"会改变"foo/bar"目录下所有源文件中代码的日志等级.
        VlogInfo(const std::string& v_switch,
            const std::string& vmodule_switch,
            int* min_log_level);
        ~VlogInfo();

        // 返回指定文件的vlog等级(通常是当前文件__FILE__).
        int GetVlogLevel(const base::StringPiece& file);

    private:
        void SetMaxVlogLevel(int level);
        int GetMaxVlogLevel() const;

        // VmodulePattern存储从|vmodule_switch|解析出的所有模式的匹配信息.
        struct VmodulePattern;
        std::vector<VmodulePattern> vmodule_levels_;
        int* min_log_level_;

        DISALLOW_COPY_AND_ASSIGN(VlogInfo);
    };

    // 如果传递的字符串和vlog模式匹配返回true. vlog模式串可以包含*和?通配符.
    // ?精确匹配一个字符, *匹配0到多个字符. /或\字符能匹配/或\.
    //
    // 举例:
    //   "kh?n"匹配"khan"  不匹配"khn"和"khaan".
    //   "kh*n"匹配"khn", "khan", "khaaaaan".
    //   "/foo\bar"匹配"/foo/bar", "\foo\bar", "/foo\bar"(不是C的转移规则).
    bool MatchVlogPattern(const base::StringPiece& string,
        const base::StringPiece& vlog_pattern);

} //namespace base

#endif //__base_vlog_h__