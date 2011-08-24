
#ifndef __ui_gfx_blit_h__
#define __ui_gfx_blit_h__

#pragma once

#include <windows.h>

class SkCanvas;

namespace gfx
{

    class Point;
    class Rect;

    // Blits a rectangle from the source context into the destination context.
    void BlitContextToContext(HDC dst_context,
        const Rect& dst_rect,
        HDC src_context,
        const Point& src_origin);

    // Blits a rectangle from the source context into the destination canvas.
    void BlitContextToCanvas(SkCanvas* dst_canvas,
        const Rect& dst_rect,
        HDC src_context,
        const Point& src_origin);

    // Blits a rectangle from the source canvas into the destination context.
    void BlitCanvasToContext(HDC dst_context,
        const Rect& dst_rect,
        SkCanvas* src_canvas,
        const Point& src_origin);

    // Blits a rectangle from the source canvas into the destination canvas.
    void BlitCanvasToCanvas(SkCanvas* dst_canvas,
        const Rect& dst_rect,
        SkCanvas* src_canvas,
        const Point& src_origin);

    // Scrolls the given subset of the given canvas by the given amount.
    // The canvas should not have a clip or a transform applied, since platforms
    // may implement those operations differently.
    void ScrollCanvas(SkCanvas* canvas,
        const Rect& clip,
        const Point& amount);

} //namespace gfx

#endif //__ui_gfx_blit_h__