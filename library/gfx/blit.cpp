
#include "blit.h"

#include "base/logging.h"

#include "skia/ext/platform_canvas.h"

#include "rect.h"

namespace gfx
{

    namespace
    {

        // Returns true if the given canvas has any part of itself clipped out or
        // any non-identity tranform.
        bool HasClipOrTransform(const skia::PlatformCanvas& canvas)
        {
            if(!canvas.getTotalMatrix().isIdentity())
            {
                return true;
            }

            const SkRegion& clip_region = canvas.getTotalClip();
            if(clip_region.isEmpty() || clip_region.isComplex())
            {
                return true;
            }

            // Now we know the clip is a regular rectangle, make sure it covers the
            // entire canvas.
            const SkBitmap& bitmap = canvas.getTopPlatformDevice().accessBitmap(false);
            const SkIRect& clip_bounds = clip_region.getBounds();
            if(clip_bounds.fLeft!=0 || clip_bounds.fTop!=0 ||
                clip_bounds.fRight!=bitmap.width() ||
                clip_bounds.fBottom!=bitmap.height())
            {
                return true;
            }

            return false;
        }

    }

    void BlitContextToContext(HDC dst_context,
        const Rect& dst_rect,
        HDC src_context,
        const Point& src_origin)
    {
        BitBlt(dst_context, dst_rect.x(), dst_rect.y(),
            dst_rect.width(), dst_rect.height(),
            src_context, src_origin.x(), src_origin.y(), SRCCOPY);
    }

    void BlitContextToCanvas(skia::PlatformCanvas* dst_canvas,
        const Rect& dst_rect,
        HDC src_context,
        const Point& src_origin)
    {
        BlitContextToContext(dst_canvas->beginPlatformPaint(), dst_rect,
            src_context, src_origin);
        dst_canvas->endPlatformPaint();
    }

    void BlitCanvasToContext(HDC dst_context,
        const Rect& dst_rect,
        skia::PlatformCanvas* src_canvas,
        const Point& src_origin)
    {
        BlitContextToContext(dst_context, dst_rect,
            src_canvas->beginPlatformPaint(), src_origin);
        src_canvas->endPlatformPaint();
    }

    void BlitCanvasToCanvas(skia::PlatformCanvas* dst_canvas,
        const Rect& dst_rect,
        skia::PlatformCanvas* src_canvas,
        const Point& src_origin)
    {
        BlitContextToContext(dst_canvas->beginPlatformPaint(), dst_rect,
            src_canvas->beginPlatformPaint(), src_origin);
        src_canvas->endPlatformPaint();
        dst_canvas->endPlatformPaint();
    }

    void ScrollCanvas(skia::PlatformCanvas* canvas,
        const gfx::Rect& clip,
        const gfx::Point& amount)
    {
        DCHECK(!HasClipOrTransform(*canvas)); // Don't support special stuff.
        HDC hdc = canvas->beginPlatformPaint();

        RECT damaged_rect;
        RECT r = clip.ToRECT();
        ScrollDC(hdc, amount.x(), amount.y(), NULL, &r, NULL, &damaged_rect);

        canvas->endPlatformPaint();
    }

} //namespace gfx