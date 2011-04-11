
#ifndef __gfx_point_h__
#define __gfx_point_h__

#pragma once

#include <iosfwd>

typedef unsigned long DWORD;
typedef struct tagPOINT POINT;

namespace gfx
{

    // (x, y)坐标点.
    class Point
    {
    public:
        Point();
        Point(int x, int y);
        // |point|用一个DWORD值表示坐标. x坐标是低位的short, y坐标是高位的short.
        // 这样的坐标值一般都是从GetMessagePos/GetCursorPos返回.
        explicit Point(DWORD point);
        explicit Point(const POINT& point);
        Point& operator=(const POINT& point);

        ~Point() {}

        int x() const { return x_; }
        int y() const { return y_; }

        void SetPoint(int x, int y)
        {
            x_ = x;
            y_ = y;
        }

        void set_x(int x) { x_ = x; }
        void set_y(int y) { y_ = y; }

        void Offset(int delta_x, int delta_y)
        {
            x_ += delta_x;
            y_ += delta_y;
        }

        Point Add(const Point& other) const
        {
            Point copy = *this;
            copy.Offset(other.x_, other.y_);
            return copy;
        }

        Point Subtract(const Point& other) const
        {
            Point copy = *this;
            copy.Offset(-other.x_, -other.y_);
            return copy;
        }

        bool operator==(const Point& rhs) const
        {
            return x_==rhs.x_ && y_==rhs.y_;
        }

        bool operator!=(const Point& rhs) const
        {
            return !(*this == rhs);
        }

        // 一个点比另外一个点小是比较谁的y值更接近原点. 如果y值相同, 接
        // 着比较谁的x值接近原点.
        // 在sets中或者vectors排序时使用Points, 需要<比较.
        bool operator<(const Point& rhs) const
        {
            return (y_==rhs.y_) ? (x_<rhs.x_) : (y_<rhs.y_);
        }

        POINT ToPOINT() const;

    private:
        int x_;
        int y_;
    };

    std::ostream& operator<<(std::ostream& out, const gfx::Point& p);

} //namespace gfx

#endif //__gfx_point_h__