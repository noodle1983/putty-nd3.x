
#ifndef __gfx_size_h__
#define __gfx_size_h__

#pragma once

#include <iosfwd>

typedef struct tagSIZE SIZE;

namespace gfx
{

    // 表示宽和高.
    class Size
    {
    public:
        Size() : width_(0), height_(0) {}
        Size(int width, int height);
        ~Size() {}

        int width() const { return width_; }
        int height() const { return height_; }

        int GetArea() const { return width_ * height_; }

        void SetSize(int width, int height)
        {
            set_width(width);
            set_height(height);
        }

        void Enlarge(int width, int height)
        {
            set_width(width_+width);
            set_height(height_+height);
        }

        void set_width(int width);
        void set_height(int height);

        bool operator==(const Size& s) const
        {
            return width_==s.width_ && height_==s.height_;
        }

        bool operator!=(const Size& s) const
        {
            return !(*this == s);
        }

        bool IsEmpty() const
        {
            // Size不允许有负值, 所以只用测试是否为0.
            return (width_==0) || (height_==0);
        }

        SIZE ToSIZE() const;

    private:
        int width_;
        int height_;
    };

    std::ostream& operator<<(std::ostream& out, const gfx::Size& s);

} //namespace gfx

#endif //__gfx_size_h__