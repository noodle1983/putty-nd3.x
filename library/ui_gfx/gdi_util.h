
#ifndef __ui_gfx_gdi_util_h__
#define __ui_gfx_gdi_util_h__

#pragma once

#include <vector>
#include <windows.h>

#include "rect.h"

namespace gfx
{

    // 根据给定的位图尺寸创建一个BITMAPINFOHEADER结构体.
    void CreateBitmapHeader(int width, int height, BITMAPINFOHEADER* hdr);

    // // 根据给定的位图尺寸和位深创建一个BITMAPINFOHEADER结构体.
    void CreateBitmapHeaderWithColorDepth(int width, int height,
        int color_depth, BITMAPINFOHEADER* hdr);

    // 根据给定的位图尺寸创建一个BITMAPV4HEADER结构体. 只有当你需要透明度(
    // alpha通道)的时候才有必要使用BMP V4. 函数设置AlphaMask为0xff000000.
    void CreateBitmapV4Header(int width, int height, BITMAPV4HEADER* hdr);

    // 创建一个黑白的BITMAPINFOHEADER.
    void CreateMonochromeBitmapHeader(int width, int height,
        BITMAPINFOHEADER* hdr);

    // 从hrgn中减去cutouts.
    void SubtractRectanglesFromRegion(HRGN hrgn,
        const std::vector<gfx::Rect>& cutouts);

} //namespace gfx

#endif //__ui_gfx_gdi_util_h__