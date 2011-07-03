
#ifndef __ui_gfx_brush_h__
#define __ui_gfx_brush_h__

#pragma once

#include "base/basic_types.h"

namespace gfx
{

    // Brush封装平台本地画刷. 子类处理底层的本地画刷内存管理.
    class Brush
    {
    public:
        Brush() {}
        virtual ~Brush() {}

    private:
        DISALLOW_COPY_AND_ASSIGN(Brush);
    };

} //namespace gfx

#endif //__ui_gfx_brush_h__