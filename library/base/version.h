
#ifndef __base_version_h__
#define __base_version_h__

#pragma once

#include <string>
#include <vector>

#include "basic_types.h"

class Version
{
public:
    // version字符串必须是由1或者多个以'.'分开的uint16组成. 不是这种格式的
    // 返回NULL. 调用者负责释放Version对象.
    static Version* GetVersionFromString(const std::string& version_str);

    // 暴露接口只是为了能存储于STL容器, 下面的方法调用都会DCHECK.
    Version();
    ~Version();

    // 创建一份拷贝. 调用者接管所有权.
    Version* Clone() const;

    bool Equals(const Version& other) const;

    // 返回 -1, 0, 1 表示 <, ==, >.
    int CompareTo(const Version& other) const;

    // 返回version字符串.
    const std::string GetString() const;

    const std::vector<uint16>& components() const { return components_; }

private:
    bool InitFromString(const std::string& version_str);

    bool is_valid_;
    std::vector<uint16> components_;
};

#endif //__base_version_h__