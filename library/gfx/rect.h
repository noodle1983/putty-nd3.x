
#ifndef __gfx_rect_h__
#define __gfx_rect_h__

#pragma once

#include "point.h"
#include "size.h"

// 定义整数类型的矩形类. 包含关系的语义有点类似数组, 即坐标(x, y)认为是包含
// 在矩形内部, 而坐标(x+width, y)不包含. 你可以创建一个非法的矩形(宽高为负
// 数), 但调用某些不允许这种情况的操作(比如Contains())时会有断言.

typedef struct tagRECT RECT;

namespace gfx
{

    class Insets;

    class Rect
    {
    public:
        Rect();
        Rect(int width, int height);
        Rect(int x, int y, int width, int height);
        explicit Rect(const RECT& r);
        explicit Rect(const gfx::Size& size);
        Rect(const gfx::Point& origin, const gfx::Size& size);

        ~Rect() {}

        Rect& operator=(const RECT& r);

        int x() const { return origin_.x(); }
        void set_x(int x) { origin_.set_x(x); }

        int y() const { return origin_.y(); }
        void set_y(int y) { origin_.set_y(y); }

        int width() const { return size_.width(); }
        void set_width(int width) { size_.set_width(width); }

        int height() const { return size_.height(); }
        void set_height(int height) { size_.set_height(height); }

        const gfx::Point& origin() const { return origin_; }
        void set_origin(const gfx::Point& origin) { origin_ = origin; }

        const gfx::Size& size() const { return size_; }
        void set_size(const gfx::Size& size) { size_ = size; }

        int right() const { return x() + width(); }
        int bottom() const { return y() + height(); }

        void SetRect(int x, int y, int width, int height);

        // 矩形的水平两边各缩小horizontal, 上下两边各缩小vertical.
        void Inset(int horizontal, int vertical)
        {
            Inset(horizontal, vertical, horizontal, vertical);
        }

        // 用insets缩小矩形的四边.
        void Inset(const gfx::Insets& insets);

        // 缩小矩形的四边.
        void Inset(int left, int top, int right, int bottom);

        // 矩形水平移动horizontal, 垂直移动vertical.
        void Offset(int horizontal, int vertical);
        void Offset(const gfx::Point& point)
        {
            Offset(point.x(), point.y());
        }

        // 如果矩形面积为0返回true.
        bool IsEmpty() const { return size_.IsEmpty(); }

        bool operator==(const Rect& other) const;

        bool operator!=(const Rect& other) const
        {
            return !(*this == other);
        }

        // 一个矩形比另一个矩形小是通过原点大小来比较的. 如果原点相同,
        // 矮的矩形比高的小. 如果原点和高度都相同, 那么窄的比宽的小.
        // 在sets中或者vectors排序时使用Rects, 需要<比较.
        bool operator<(const Rect& other) const;

        // 构造一个等价的Win32 RECT对象.
        RECT ToRECT() const;

        // 如果(point_x, point_y)点落在矩形中返回true. 点(x, y)在矩形内, 但是点
        // (x+width, y+height)没有.
        bool Contains(int point_x, int point_y) const;

        // 如果矩形包含点point, 返回true.
        bool Contains(const gfx::Point& point) const
        {
            return Contains(point.x(), point.y());
        }

        // 如果矩形包含rect, 返回true.
        bool Contains(const Rect& rect) const;

        // 如果矩形和rect相交, 返回true.
        bool Intersects(const Rect& rect) const;

        // 计算矩形和rect的交集.
        Rect Intersect(const Rect& rect) const;

        // 计算矩形和rect的并集. 并集是两个矩形的最小外包矩形.
        Rect Union(const Rect& rect) const;

        // 计算和|rect|的减集. 如果不和|rect|相交, 返回|*this|. 如果被|rect|包含,
        // 返回空矩形.
        Rect Subtract(const Rect& rect) const;

        // 如果相等, 返回true.
        bool Equals(const Rect& rect) const
        {
            return *this == rect;
        }

        // 调整矩形以使其尽可能多的落在rect当中, 返回调整后的矩形. 比如, 如果当前
        // 矩形x坐标为2宽为4, rect矩形x坐标为0宽为5, 返回的矩形x坐标为1宽为4.
        Rect AdjustToFit(const Rect& rect) const;

        // 返回矩形的中心点.
        Point CenterPoint() const;

        // 返回一个同心矩形, 尺寸为|size|.
        Rect Center(const gfx::Size& size) const;

        // 如果矩形和rect共享整条边(比如长度或者宽度相等), 两个矩形不重叠, 返回true.
        bool SharesEdgeWith(const gfx::Rect& rect) const;

    private:
        gfx::Point origin_;
        gfx::Size size_;
    };

    std::ostream& operator<<(std::ostream& out, const gfx::Rect& r);

} //namespace gfx

#endif //__gfx_rect_h__