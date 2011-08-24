
#ifndef __skia_skia_utils_win_h__
#define __skia_skia_utils_win_h__

#pragma once

#include "SkColor.h"

struct SkIRect;
struct SkPoint;
struct SkRect;
typedef unsigned long DWORD;
typedef DWORD COLORREF;
typedef struct tagPOINT POINT;
typedef struct tagRECT RECT;

namespace skia
{

    // Skia的点转换成Windows的POINT.
    POINT SkPointToPOINT(const SkPoint& point);

    // Windows的RECT转换成Skia的矩形.
    SkRect RECTToSkRect(const RECT& rect);

    // Windows的RECT转换成Skia的矩形.
    // 两者使用相同的内存格式. 在skia_utils.cpp中通过COMPILE_ASSERT()
    // 验证.
    inline const SkIRect& RECTToSkIRect(const RECT& rect)
    {
        return reinterpret_cast<const SkIRect&>(rect);
    }

    // Skia的矩形转换成Windows的RECT.
    // 两者使用相同的内存格式. 在skia_utils.cpp中通过COMPILE_ASSERT()
    // 验证.
    inline const RECT& SkIRectToRECT(const SkIRect& rect)
    {
        return reinterpret_cast<const RECT&>(rect);
    }

    // 转换COLORREFs(0BGR)到Skia支持的ARGB排列方式.
    SkColor COLORREFToSkColor(COLORREF color);

    // 转换ARGB到COLORREFs(0BGR).
    COLORREF SkColorToCOLORREF(SkColor color);

} // namespace skia

#endif //__skia_skia_utils_win_h__