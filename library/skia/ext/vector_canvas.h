
#ifndef __skia_ext_vector_canvas_h__
#define __skia_ext_vector_canvas_h__

#pragma once

#include "platform_canvas.h"

namespace skia
{

    // PlatformCanvas是一个特殊化的规则SkCanvas, 用于配合VectorDevice来管理平台
    // 相关的绘图. 它同时允许Skia操作和平台相关操作. 它不支持位图双缓存因为没有
    // 使用位图.
    class VectorCanvas : public PlatformCanvas
    {
    public:
        VectorCanvas();
        explicit VectorCanvas(SkDeviceFactory* factory);
        VectorCanvas(HDC dc, int width, int height);
        virtual ~VectorCanvas();

        // 如果你使用的是上面无参数的构造函数, 这是第二步初始化工作.
        bool initialize(HDC context, int width, int height);

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