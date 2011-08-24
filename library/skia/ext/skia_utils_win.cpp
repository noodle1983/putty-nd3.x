
#include "skia_utils_win.h"

#include <windows.h>

#include "SkRect.h"
#include "SkGradientShader.h"

namespace
{

    template<bool>
    struct CompileAssert {};

#undef COMPILE_ASSERT
#define COMPILE_ASSERT(expr, msg) \
    typedef CompileAssert<(bool(expr))> msg[bool(expr)?1:-1]

    COMPILE_ASSERT(SK_OFFSETOF(RECT, left) == SK_OFFSETOF(SkIRect, fLeft), o1);
    COMPILE_ASSERT(SK_OFFSETOF(RECT, top) == SK_OFFSETOF(SkIRect, fTop), o2);
    COMPILE_ASSERT(SK_OFFSETOF(RECT, right) == SK_OFFSETOF(SkIRect, fRight), o3);
    COMPILE_ASSERT(SK_OFFSETOF(RECT, bottom) == SK_OFFSETOF(SkIRect, fBottom), o4);
    COMPILE_ASSERT(sizeof(RECT().left) == sizeof(SkIRect().fLeft), o5);
    COMPILE_ASSERT(sizeof(RECT().top) == sizeof(SkIRect().fTop), o6);
    COMPILE_ASSERT(sizeof(RECT().right) == sizeof(SkIRect().fRight), o7);
    COMPILE_ASSERT(sizeof(RECT().bottom) == sizeof(SkIRect().fBottom), o8);
    COMPILE_ASSERT(sizeof(RECT) == sizeof(SkIRect), o9);

}

namespace skia
{

    POINT SkPointToPOINT(const SkPoint& point)
    {
        POINT win_point = { SkScalarRound(point.fX), SkScalarRound(point.fY) };
        return win_point;
    }

    SkRect RECTToSkRect(const RECT& rect)
    {
        SkRect sk_rect =
        {
            SkIntToScalar(rect.left), SkIntToScalar(rect.top),
            SkIntToScalar(rect.right), SkIntToScalar(rect.bottom)
        };
        return sk_rect;
    }

    SkColor COLORREFToSkColor(COLORREF color)
    {
#ifndef _MSC_VER
        return SkColorSetRGB(GetRValue(color), GetGValue(color), GetBValue(color));
#else
        // ARGB = 0xFF000000 | ((0BGR -> RGB0) >> 8)
        return 0xFF000000u | (_byteswap_ulong(color) >> 8);
#endif
    }

    COLORREF SkColorToCOLORREF(SkColor color)
    {
        // 一般地, Alpha都是255, 否则颜色值为0, 所以没必要与通道值做除法运算.
        // 如果DCHECK()被触发了, 转换时必须加上(SkColorGetX(color) * 255 / a).
        SkASSERT((0xFF==SkColorGetA(color)) || (0==color));
#ifndef _MSC_VER
        return RGB(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color));
#else
        // 0BGR = ((ARGB -> BGRA) >> 8)
        return (_byteswap_ulong(color) >> 8);
#endif
    }

} //namespace skia