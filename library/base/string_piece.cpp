
#include "string_piece.h"

#include <algorithm>

std::ostream& operator<<(std::ostream& o, const base::StringPiece& piece)
{
    o.write(piece.data(), static_cast<std::streamsize>(piece.size()));
    return o;
}

namespace base
{

    typedef StringPiece::size_type size_type;

    bool operator==(const StringPiece& x, const StringPiece& y)
    {
        if(x.size() != y.size())
        {
            return false;
        }

        return StringPiece::wordmemcmp(x.data(), y.data(), x.size()) == 0;
    }

    void StringPiece::CopyToString(std::string& target) const
    {
        target.assign(!empty()?data():"", size());
    }

    void StringPiece::AppendToString(std::string& target) const
    {
        if(!empty())
        {
            target.append(data(), size());
        }
    }

    size_type StringPiece::copy(char* buf, size_type n, size_type pos) const
    {
        size_type ret = std::min(length_-pos, n);
        memcpy(buf, ptr_+pos, ret);
        return ret;
    }

    size_type StringPiece::find(const StringPiece& s, size_type pos) const
    {
        if(pos > length_)
        {
            return npos;
        }

        const char* result = std::search(ptr_+pos, ptr_+length_,
            s.ptr_, s.ptr_+s.length_);
        const size_type xpos = result - ptr_;
        return (xpos+s.length_ <= length_) ? xpos : npos;
    }

    size_type StringPiece::find(char c, size_type pos) const
    {
        if(pos >= length_)
        {
            return npos;
        }

        const char* result = std::find(ptr_+pos, ptr_+length_, c);
        return (result != ptr_+length_) ? static_cast<size_t>(result-ptr_) : npos;
    }

    size_type StringPiece::rfind(const StringPiece& s, size_type pos) const
    {
        if(length_ < s.length_)
        {
            return npos;
        }

        if(s.empty())
        {
            return std::min(length_, pos);
        }

        const char* last = ptr_ + std::min(length_-s.length_, pos) + s.length_;
        const char* result = std::find_end(ptr_, last, s.ptr_, s.ptr_+s.length_);
        return (result != last) ? static_cast<size_t>(result-ptr_) : npos;
    }

    size_type StringPiece::rfind(char c, size_type pos) const
    {
        if(length_ == 0)
        {
            return npos;
        }

        for(size_type i=std::min(pos, length_-1); ; --i)
        {
            if(ptr_[i] == c)
            {
                return i;
            }
            if(i == 0)
            {
                break;
            }
        }
        return npos;
    }

    // characters_wanted中的每个字符, 将其ASCII码作为table索引设置值为1.
    // 在find_.*_of函数中使用, 用于在常量时间内判断一个字符是否在查找表中.
    // 'table'数组大小必须能够容纳所有的unsigned char值, 所以应声明如下:
    //     bool table[UCHAR_MAX+1]
    static inline void BuildLookupTable(const StringPiece& characters_wanted,
        bool* table)
    {
        const size_type length = characters_wanted.length();
        const char* const data = characters_wanted.data();
        for(size_type i=0; i<length; ++i)
        {
            table[static_cast<unsigned char>(data[i])] = true;
        }
    }

    size_type StringPiece::find_first_of(const StringPiece& s,
        size_type pos) const
    {
        if(length_==0 || s.length_==0)
        {
            return npos;
        }

        // 单字符查找避免BuildLookupTable()损耗
        if(s.length_ == 1)
        {
            return find_first_of(s.ptr_[0], pos);
        }

        bool lookup[UCHAR_MAX+1] = { false };
        BuildLookupTable(s, lookup);
        for(size_type i=pos; i<length_; ++i)
        {
            if(lookup[static_cast<unsigned char>(ptr_[i])])
            {
                return i;
            }
        }
        return npos;
    }

    size_type StringPiece::find_first_not_of(const StringPiece& s,
        size_type pos) const
    {
        if(length_ == 0)
        {
            return npos;
        }

        if(s.length_ == 0)
        {
            return 0;
        }

        // 单字符查找避免BuildLookupTable()损耗
        if(s.length_ == 1)
        {
            return find_first_not_of(s.ptr_[0], pos);
        }

        bool lookup[UCHAR_MAX+1] = { false };
        BuildLookupTable(s, lookup);
        for(size_type i=pos; i<length_; ++i)
        {
            if(!lookup[static_cast<unsigned char>(ptr_[i])])
            {
                return i;
            }
        }
        return npos;
    }

    size_type StringPiece::find_first_not_of(char c, size_type pos) const
    {
        if(length_ == 0)
        {
            return npos;
        }

        for(; pos<length_; ++pos)
        {
            if(ptr_[pos] != c)
            {
                return pos;
            }
        }
        return npos;
    }

    size_type StringPiece::find_last_of(const StringPiece& s, size_type pos) const
    {
        if(length_==0 || s.length_==0)
        {
            return npos;
        }

        // 单字符查找避免BuildLookupTable()损耗
        if(s.length_ == 1)
        {
            return find_last_of(s.ptr_[0], pos);
        }

        bool lookup[UCHAR_MAX+1] = { false };
        BuildLookupTable(s, lookup);
        for(size_type i=std::min(pos, length_-1); ; --i)
        {
            if(lookup[static_cast<unsigned char>(ptr_[i])])
            {
                return i;
            }
            if(i == 0)
            {
                break;
            }
        }
        return npos;
    }

    size_type StringPiece::find_last_not_of(const StringPiece& s,
        size_type pos) const
    {
        if(length_ == 0)
        {
            return npos;
        }

        size_type i = std::min(pos, length_-1);
        if(s.length_ == 0)
        {
            return i;
        }

        // 单字符查找避免BuildLookupTable()损耗
        if(s.length_ == 1)
        {
            return find_last_not_of(s.ptr_[0], pos);
        }

        bool lookup[UCHAR_MAX+1] = { false };
        BuildLookupTable(s, lookup);
        for(; ; --i)
        {
            if(!lookup[static_cast<unsigned char>(ptr_[i])])
            {
                return i;
            }
            if(i == 0)
            {
                break;
            }
        }
        return npos;
    }

    size_type StringPiece::find_last_not_of(char c, size_type pos) const
    {
        if(length_ == 0)
        {
            return npos;
        }

        for(size_type i=std::min(pos, length_-1); ; --i)
        {
            if(ptr_[i] != c)
            {
                return i;
            }
            if(i == 0)
            {
                break;
            }
        }
        return npos;
    }

    StringPiece StringPiece::substr(size_type pos, size_type n) const
    {
        if(pos > length_) pos = length_;
        if(n > length_-pos) n = length_ - pos;
        return StringPiece(ptr_+pos, n);
    }

    const StringPiece::size_type StringPiece::npos = size_type(-1);

} //namespace base