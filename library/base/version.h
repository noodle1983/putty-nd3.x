
#ifndef __base_version_h__
#define __base_version_h__

#pragma once

#include <string>
#include <vector>

#include "basic_types.h"

class Version
{
public:
    // The only thing you can legally do to a default constructed
    // Version object is assign to it.
    Version();
    ~Version();

    // Initializes from a decimal dotted version number, like "0.1.1".
    // Each component is limited to a uint16. Call IsValid() to learn
    // the outcome.
    explicit Version(const std::string& version_str);

    // Returns true if the object contains a valid version number.
    bool IsValid() const;

    // Commonly used pattern. Given a valid version object, compare if a
    // |version_str| results in a newer version. Returns true if the
    // string represents valid version and if the version is greater than
    // than the version of this object.
    bool IsOlderThan(const std::string& version_str) const;

    bool Equals(const Version& other) const;

    // 返回 -1, 0, 1 表示 <, ==, >.
    int CompareTo(const Version& other) const;

    // 返回version字符串.
    const std::string GetString() const;

    const std::vector<uint16>& components() const { return components_; }

private:
    std::vector<uint16> components_;
};

#endif //__base_version_h__