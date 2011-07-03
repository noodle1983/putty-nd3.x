
#ifndef __ui_gfx_path_h__
#define __ui_gfx_path_h__

#pragma once

#include "base/basic_types.h"

#include "SkPath.h"

#include "point.h"

namespace gfx
{

    class Path : public SkPath
    {
    public:
        Path();
        Path(const Point* points, size_t count);
        ~Path();

        // 从SkPath创建一个HRGN. 调用者负责释放返回的资源. 只支持多边形路径.
        HRGN CreateNativeRegion() const;

        // 返回两个区的交集. 调用者拥有返回对象的所有权.
        static HRGN IntersectRegions(HRGN r1, HRGN r2);

        // 返回两个区的并集. 调用者拥有返回对象的所有权.
        static HRGN CombineRegions(HRGN r1, HRGN r2);

        // 返回两个区的差集. 调用者拥有返回对象的所有权.
        static HRGN SubtractRegion(HRGN r1, HRGN r2);

    private:
        DISALLOW_COPY_AND_ASSIGN(Path);
    };

} //namespace gfx

#endif //__ui_gfx_path_h__