
#ifndef __ui_gfx_canvas_skia_h__
#define __ui_gfx_canvas_skia_h__

#pragma once

#include "canvas.h"
#include "canvas_paint_win.h"

namespace gfx
{

    // CanvasSkia是SkCanvas的子类, 为应用程序提供一组公共操作方法.
    //
    // 所有方法接受整数参数(由于在整个view框架中使用), 以Int结尾. 如果需要
    // 使用父类的方法, 需要做一个转换, 一般需要使用SkIntToScalar(xxx)宏,
    // scalar转换到integer可以使用SkScalarRound.
    //
    // 有少量的方法提供了SkXfermode::Mode类型附加参数的重载. SkXfermode::Mode
    // 指定源和目的颜色组合方式. 如无特别说明, 不带SkXfermode::Mode参数的版本
    // 使用kSrcOver_Mode转移模式.
    class CanvasSkia : public skia::PlatformCanvas, public Canvas
    {
    public:
        enum TruncateFadeMode
        {
            TruncateFadeTail,
            TruncateFadeHead,
            TruncateFadeHeadAndTail,
        };

        // 创建一个空的Canvas. 调用者在使用前必须初始化.
        CanvasSkia();

        CanvasSkia(int width, int height, bool is_opaque);

        virtual ~CanvasSkia();

        // 计算用提供字体绘制文本所需的尺寸. 尝试调整宽和高来适应文本, 根据需要
        // 增加高和宽. 方法支持多行文本.
        static void SizeStringInt(const string16& text,
            const Font& font,
            int* width, int* height,
            int flags);

        // 基于本地系统语言的方向性, 返回gfx::CanvasSkia绘制文本时使用的缺省文本
        // 对齐方式. gfx::Canvas::DrawStringInt在没指定对齐方式时会调用本函数.
        //
        // 返回gfx::Canvas::TEXT_ALIGN_LEFT、gfx::Canvas::TEXT_ALIGN_RIGHT之一.
        static int DefaultCanvasTextAlignment();

        // 用给定颜色绘制带有1像素光圈的文本. 允许绘制ClearType到一个拖拽图像的
        // 透明位图上. 拖拽图像的透明度只有1-bit, 因此不做任何模糊化.
        void DrawStringWithHalo(const string16& text,
            const Font& font,
            const SkColor& text_color,
            const SkColor& halo_color,
            int x, int y, int w, int h,
            int flags);

        // 提取画布内容成一个位图.
        SkBitmap ExtractBitmap() const;

        // 覆盖重载Canvas:
        virtual void Save();
        virtual void SaveLayerAlpha(uint8 alpha);
        virtual void SaveLayerAlpha(uint8 alpha, const Rect& layer_bounds);
        virtual void Restore();
        virtual bool ClipRectInt(int x, int y, int w, int h);
        virtual void TranslateInt(int x, int y);
        virtual void ScaleInt(int x, int y);
        virtual void FillRectInt(const SkColor& color, int x, int y, int w, int h);
        virtual void FillRectInt(const SkColor& color, int x, int y, int w, int h,
            SkXfermode::Mode mode);
        virtual void FillRectInt(const Brush* brush, int x, int y, int w, int h);
        virtual void DrawRectInt(const SkColor& color, int x, int y, int w, int h);
        virtual void DrawRectInt(const SkColor& color, int x, int y, int w, int h,
            SkXfermode::Mode mode);
        virtual void DrawRectInt(int x, int y, int w, int h, const SkPaint& paint);
        virtual void DrawLineInt(const SkColor& color, int x1, int y1, int x2, int y2);
        virtual void DrawBitmapInt(const SkBitmap& bitmap, int x, int y);
        virtual void DrawBitmapInt(const SkBitmap& bitmap, int x, int y,
            const SkPaint& paint);
        virtual void DrawBitmapInt(const SkBitmap& bitmap,
            int src_x, int src_y, int src_w, int src_h,
            int dest_x, int dest_y, int dest_w, int dest_h,
            bool filter);
        virtual void DrawBitmapInt(const SkBitmap& bitmap,
            int src_x, int src_y, int src_w, int src_h,
            int dest_x, int dest_y, int dest_w, int dest_h,
            bool filter,
            const SkPaint& paint);
        virtual void DrawStringInt(const string16& text,
            const Font& font,
            const SkColor& color,
            int x, int y, int w, int h);
        virtual void DrawStringInt(const string16& text,
            const Font& font,
            const SkColor& color,
            const Rect& display_rect);
        virtual void DrawStringInt(const string16& text,
            const Font& font,
            const SkColor& color,
            int x, int y, int w, int h,
            int flags);
        // Draws the given string with the beginning and/or the end using a fade
        // gradient. When truncating the head
        // |desired_characters_to_truncate_from_head| specifies the maximum number of
        // characters that can be truncated.
        virtual void DrawFadeTruncatingString(
            const string16& text,
            TruncateFadeMode truncate_mode,
            size_t desired_characters_to_truncate_from_head,
            const gfx::Font& font,
            const SkColor& color,
            const gfx::Rect& display_rect);
        virtual void DrawFocusRect(int x, int y, int width, int height);
        virtual void TileImageInt(const SkBitmap& bitmap, int x, int y, int w, int h);
        virtual void TileImageInt(const SkBitmap& bitmap,
            int src_x, int src_y, int dest_x, int dest_y, int w, int h);
        virtual HDC BeginPlatformPaint();
        virtual void EndPlatformPaint();
        virtual void ConcatTransform(const Transform& transform);
        virtual CanvasSkia* AsCanvasSkia();
        virtual const CanvasSkia* AsCanvasSkia() const;

    private:
        // 测试给定的矩形是否与当前裁剪区相交.
        bool IntersectsClipRectInt(int x, int y, int w, int h);

        // 用特定颜色、字体在给定的位置绘制文本. 文本水平方向左对齐, 垂直方向
        // 居中对齐, 在区域中裁剪. 如果文本太大, 会截取并在末尾添加'...'.
        void DrawStringInt(const string16& text,
            HFONT font,
            const SkColor& color,
            int x, int y, int w, int h,
            int flags);

        DISALLOW_COPY_AND_ASSIGN(CanvasSkia);
    };

    typedef CanvasPaintT<CanvasSkia> CanvasSkiaPaint;

} //namespace gfx

#endif //__ui_gfx_canvas_skia_h__