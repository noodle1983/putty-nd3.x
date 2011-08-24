
#include "version.h"

#include "logging.h"
#include "string_number_conversions.h"
#include "string_split.h"
#include "string_util.h"
#include "utf_string_conversions.h"

Version::Version() {}

Version::~Version() {}

Version::Version(const std::string& version_str)
{
    std::vector<std::string> numbers;
    base::SplitString(version_str, '.', &numbers);
    if(numbers.empty())
    {
        return;
    }
    std::vector<uint16> parsed;
    for(std::vector<std::string>::iterator i=numbers.begin();
        i!=numbers.end(); ++i)
    {
        int num;
        if(!base::StringToInt(*i, &num))
        {
            return;
        }
        if(num < 0)
        {
            return;
        }
        const uint16 max = 0xFFFF;
        if(num > max)
        {
            return;
        }
        // This throws out things like +3, or 032.
        if(base::IntToString(num) != *i)
        {
            return;
        }
        parsed.push_back(static_cast<uint16>(num));
    }
    components_.swap(parsed);
}

bool Version::IsValid() const
{
    return (!components_.empty());
}

bool Version::IsOlderThan(const std::string& version_str) const
{
    Version proposed_ver(version_str);
    if(!proposed_ver.IsValid())
    {
        return false;
    }
    return (CompareTo(proposed_ver) < 0);
}

bool Version::Equals(const Version& that) const
{
    DCHECK(IsValid());
    DCHECK(that.IsValid());
    return (CompareTo(that) == 0);
}

int Version::CompareTo(const Version& other) const
{
    DCHECK(IsValid());
    DCHECK(other.IsValid());
    size_t count = std::min(components_.size(), other.components_.size());
    for(size_t i=0; i<count; ++i)
    {
        if(components_[i] > other.components_[i])
        {
            return 1;
        }
        if(components_[i] < other.components_[i])
        {
            return -1;
        }
    }
    if(components_.size() > other.components_.size())
    {
        for(size_t i=count; i<components_.size(); ++i)
        {
            if(components_[i] > 0)
            {
                return 1;
            }
        }
    }
    else if(components_.size() < other.components_.size())
    {
        for(size_t i=count; i<other.components_.size(); ++i)
        {
            if(other.components_[i] > 0)
            {
                return -1;
            }
        }
    }
    return 0;
}

const std::string Version::GetString() const
{
    DCHECK(IsValid());
    std::string version_str;
    int count = components_.size();
    for(int i=0; i<count-1; ++i)
    {
        version_str.append(base::IntToString(components_[i]));
        version_str.append(".");
    }
    version_str.append(base::IntToString(components_[count-1]));
    return version_str;
}