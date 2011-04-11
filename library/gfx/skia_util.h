
#ifndef __gfx_skia_util_h__
#define __gfx_skia_util_h__

#pragma once

#include "SkColor.h"
#include "SkRect.h"

class SkBitmap;
class SkShader;

namespace gfx
{

    class Rect;

    // Skia矩形和gfx矩形之间转换.
    SkRect RectToSkRect(const gfx::Rect& rect);
    gfx::Rect SkRectToRect(const SkRect& rect);

    // 创建一个垂直渐变着色器, 调用者拥有返回对象.
    // 避免泄露的示例:
    //     SkSafeUnref(paint.setShader(gfx::CreateGradientShader(0, 10, red, blue)));
    //
    // (如果paint中有旧的着色器, 需要释放, SkSafeUnref会处理空置的情况).
    SkShader* CreateGradientShader(int start_point, int end_point,
        SkColor start_color, SkColor end_color);

    // 如果两个位图像素一样返回true.
    bool BitmapsAreEqual(const SkBitmap& bitmap1, const SkBitmap& bitmap2);

} //namespace gfx

#endif //__gfx_skia_util_h__