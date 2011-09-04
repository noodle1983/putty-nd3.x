
#ifndef __skia_vector_platform_device_emf_win_h__
#define __skia_vector_platform_device_emf_win_h__

#pragma once

#include "base/basic_types.h"

#include "SkMatrix.h"
#include "SkRegion.h"

#include "platform_device_win.h"

namespace skia
{

    // VectorPlatformDeviceEmf是SkBitmap的基本封装, 为SkCanvas提供绘图表面. 它没有
    // 内存缓冲, 因此是不可读的. 因为后台缓冲是完全矢量化的, VectorPlatformDeviceEmf
    // 只是对Windows DC句柄的封装.
    class VectorPlatformDeviceEmf : public PlatformDevice, public SkDevice
    {
    public:
        static SkDevice* CreateDevice(int width, int height,
            bool isOpaque, HANDLE shared_section);

        // 类厂函数. DC被保存作为输出环境.
        static SkDevice* create(HDC dc, int width, int height);

        VectorPlatformDeviceEmf(HDC dc, const SkBitmap& bitmap);
        virtual ~VectorPlatformDeviceEmf();

        // PlatformDevice methods
        virtual HDC BeginPlatformPaint();
        virtual void DrawToNativeContext(HDC dc, int x, int y,
            const RECT* src_rect);
        virtual bool AlphaBlendUsed() const { return alpha_blend_used_; }

        // SkDevice methods.
        virtual uint32_t getDeviceCapabilities();
        virtual void drawPaint(const SkDraw& draw, const SkPaint& paint);
        virtual void drawPoints(const SkDraw& draw, SkCanvas::PointMode mode,
            size_t count, const SkPoint[], const SkPaint& paint);
        virtual void drawRect(const SkDraw& draw, const SkRect& r,
            const SkPaint& paint);
        virtual void drawPath(const SkDraw& draw, const SkPath& path,
            const SkPaint& paint,
            const SkMatrix* prePathMatrix=NULL,
            bool pathIsMutable=false);
        virtual void drawBitmap(const SkDraw& draw, const SkBitmap& bitmap,
            const SkIRect* srcRectOrNull,
            const SkMatrix& matrix,
            const SkPaint& paint);
        virtual void drawSprite(const SkDraw& draw, const SkBitmap& bitmap,
            int x, int y, const SkPaint& paint);
        virtual void drawText(const SkDraw& draw, const void* text, size_t len,
            SkScalar x, SkScalar y, const SkPaint& paint);
        virtual void drawPosText(const SkDraw& draw, const void* text, size_t len,
            const SkScalar pos[], SkScalar constY,
            int scalarsPerPos, const SkPaint& paint);
        virtual void drawTextOnPath(const SkDraw& draw, const void* text, size_t len,
            const SkPath& path, const SkMatrix* matrix,
            const SkPaint& paint);
        virtual void drawVertices(const SkDraw& draw, SkCanvas::VertexMode,
            int vertexCount,
            const SkPoint verts[], const SkPoint texs[],
            const SkColor colors[], SkXfermode* xmode,
            const uint16_t indices[], int indexCount,
            const SkPaint& paint);
        virtual void drawDevice(const SkDraw& draw, SkDevice*, int x, int y,
            const SkPaint&);

        virtual void setMatrixClip(const SkMatrix& transform, const SkRegion& region,
            const SkClipStack&);

        void LoadClipRegion();

    protected:
        virtual SkDevice* onCreateCompatibleDevice(SkBitmap::Config, int width,
            int height, bool isOpaque);

    private:
        // 应用SkPaint的绘图属性到当前的GDI设备环境. 如果GDI不支持所有的绘图属性,
        // 函数返回false. 不执行SkPaint中的"命令".
        bool ApplyPaint(const SkPaint& paint);

        // 选择一个新对象到设备环境. 可以是画笔、画刷、裁剪区、位图或者字体.
        // 返回选出的对象.
        HGDIOBJ SelectObject(HGDIOBJ object);

        // 根据SkPaint的属性创建一个画刷.
        bool CreateBrush(bool use_brush, const SkPaint& paint);

        // 根据SkPaint的属性创建一个画笔.
        bool CreatePen(bool use_pen, const SkPaint& paint);

        // 在执行绘图命令后恢复到以前的对象(画笔、画刷等).
        void Cleanup();

        // 创建一个画笔.
        bool CreateBrush(bool use_brush, COLORREF color);

        // 根据SkPaint的属性创建一个画笔.
        bool CreatePen(bool use_pen, COLORREF color, int stroke_width,
            float stroke_miter, DWORD pen_style);

        // 在设备中绘制位图, 使用当前加载的变换矩阵.
        void InternalDrawBitmap(const SkBitmap& bitmap, int x, int y,
            const SkPaint& paint);

        // Windows DC句柄. GDI绘图的设备环境, 是只写的且矢量的.
        HDC hdc_;

        // DC的变换矩阵: 需要单独维护这个变量, 因为设备环境还没创建的时候
        // 可能已经需要更新这个值.
        SkMatrix transform_;

        // 当前裁剪区.
        SkRegion clip_region_;

        // 当前绘制的旧画刷.
        HGDIOBJ previous_brush_;

        // 当前绘制的旧画笔.
        HGDIOBJ previous_pen_;

        // 打印时调用过AlphaBlend()则为true.
        bool alpha_blend_used_;

        DISALLOW_COPY_AND_ASSIGN(VectorPlatformDeviceEmf);
    };

} //namespace skia

#endif //__skia_vector_platform_device_emf_win_h__