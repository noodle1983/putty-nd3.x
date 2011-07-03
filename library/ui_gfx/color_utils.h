
#ifndef __ui_gfx_color_utils_h__
#define __ui_gfx_color_utils_h__

#pragma once

#include "SkColor.h"

class SkBitmap;

namespace gfx
{

    // HSL颜色.
    struct HSL
    {
        double h;
        double s;
        double l;
    };

    unsigned char GetLuminanceForColor(SkColor color);

    // 按照 http://www.w3.org/TR/WCAG20/#relativeluminancedef 计算.
    double RelativeLuminance(SkColor color);

    // 注意: 这些转换假定源颜色空间是sRGB.
    void SkColorToHSL(SkColor c, HSL* hsl);
    SkColor HSLToSkColor(const HSL& hsl, SkAlpha alpha);

    // HSL-Shift an SkColor. The shift values are in the range of 0-1, with the
    // option to specify -1 for 'no change'. The shift values are defined as:
    // hsl_shift[0] (hue): The absolute hue value - 0 and 1 map
    //    to 0 and 360 on the hue color wheel (red).
    // hsl_shift[1] (saturation): A saturation shift, with the
    //    following key values:
    //    0 = remove all color.
    //    0.5 = leave unchanged.
    //    1 = fully saturate the image.
    // hsl_shift[2] (lightness): A lightness shift, with the
    //    following key values:
    //    0 = remove all lightness (make all pixels black).
    //    0.5 = leave unchanged.
    //    1 = full lightness (make all pixels white).
    SkColor HSLShift(SkColor color, const HSL& shift);

    // 确定给定的alpha值是否接近完全透明.
    bool IsColorCloseToTransparent(SkAlpha alpha);

    // 确定颜色是否接近灰度色.
    bool IsColorCloseToGrey(int r, int g, int b);

    // 得到位图的代表色, "代表色"的定义是位图颜色的平均值, 其alpha值通过|alpha|
    // 指定.
    SkColor GetAverageColorOfFavicon(SkBitmap* bitmap, SkAlpha alpha);

    // 基于图像Y'UV表示形式中的Y'构建一个直方图.
    void BuildLumaHistogram(SkBitmap* bitmap, int histogram[256]);

    // 返回融合色, 范围从|background|(|alpha|==0)到|foreground|(|alpha|==255).
    SkColor AlphaBlend(SkColor foreground, SkColor background, SkAlpha alpha);

    // 给定前景色和背景色, 尝试返回一个前景色, 保证在背景色下是可识别的. 采用的
    // 方法是计算前景色的亮度和反相亮度, 选择和背景色对比度高的.
    //
    // 注意: 当前景色的亮度值接近中点(HSL中用0.5表示)时, 调用此函数只会浪费时间,
    // 没有任何效果.
    SkColor GetReadableColor(SkColor foreground, SkColor background);

    // 获取SkColor类型的Windows系统色.
    SkColor GetSysSkColor(int which);

} //namespace gfx

#endif //__ui_gfx_color_utils_h__