
#ifndef __skia_ext_vector_canvas_h__
#define __skia_ext_vector_canvas_h__

#pragma once

#include "platform_canvas.h"

namespace skia
{

    class PlatformDevice;

    // VectorCanvas是一个特殊的PlatformCanvas, 用于配合VectorDevice来管理平台
    // 相关的绘图. 它同时允许Skia操作和平台相关操作. 不支持位图双缓存因为没有
    // 使用位图.
    class VectorCanvas : public PlatformCanvas
    {
    public:
        explicit VectorCanvas(PlatformDevice* device);
        virtual ~VectorCanvas();

        virtual SkBounder* setBounder(SkBounder* bounder);
        virtual SkDrawFilter* setDrawFilter(SkDrawFilter* filter);

    private:
        // 如果顶层设备是基于向量而不是位图的, 返回true.
        bool IsTopDeviceVectorial() const;

        // 不支持拷贝和赋值构造函数.
        VectorCanvas(const VectorCanvas&);
        const VectorCanvas& operator=(const VectorCanvas&);
    };

} //namespace skia

#endif //__skia_ext_vector_canvas_h__