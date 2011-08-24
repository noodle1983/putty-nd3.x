
#ifndef __skia_platform_canvas_h__
#define __skia_platform_canvas_h__

#pragma once

#include "SkCanvas.h"

#include "platform_device_win.h"

namespace skia
{

    // PlatformCanvas是一个特殊化的规则SkCanvas, 用于配合PlatformDevice来管理平台
    // 相关的绘图. 它同时允许Skia操作和平台相关操作.
    class PlatformCanvas : public SkCanvas
    {
    public:
        // 如果使用无参数版本的构造函数, 必须手动调用initialize().
        PlatformCanvas();
        // 如果想要擦除位图, 且不需要透明度, 可以设置is_opaque: 这么做会有一些
        // 优化.
        PlatformCanvas(int width, int height, bool is_opaque);
        // 传递shared_section给BitmapPlatformDevice::create.
        PlatformCanvas(int width, int height, bool is_opaque, HANDLE shared_section);
        
        virtual ~PlatformCanvas();

        // 如果你使用的是上面无参数的构造函数, 这是第二步初始化工作.
        bool initialize(int width, int height, bool is_opaque,
            HANDLE shared_section=NULL);

        // 返回给定宽度的stride值(一行所用的字节数). 因为每个像素使用32-bits, 所以
        // 大概是4*width. 然而, 由于对齐的原因可能还会增加.
        static size_t StrideForWidth(unsigned width);

    private:
        // initialize()方法调用的辅助函数.
        bool initializeWithDevice(SkDevice* device);

        // 未实现. 用于阻止他人调用SkCanvas的这个函数. SkCanvas的版本不是虚函数, 所
        // 以我们并不能100%的阻止, 但希望它能引起人们的注意, 不再使用这个函数. 调用
        // SkCanvas的版本会创建一个新的不兼容设备, 如果有人想使用CoreGraphics在里面
        // 绘图, 将会崩溃.
        virtual SkDevice* setBitmapDevice(const SkBitmap& bitmap);

        // 禁止拷贝和赋值构造函数.
        PlatformCanvas(const PlatformCanvas&);
        PlatformCanvas& operator=(const PlatformCanvas&);
    };

    // Returns the SkDevice pointer of the topmost rect with a non-empty
    // clip. In practice, this is usually either the top layer or nothing, since
    // we usually set the clip to new layers when we make them.
    //
    // If there is no layer that is not all clipped out, this will return a
    // dummy device so callers do not have to check. If you are concerned about
    // performance, check the clip before doing any painting.
    //
    // This is different than SkCanvas' getDevice, because that returns the
    // bottommost device.
    //
    // Danger: the resulting device should not be saved. It will be invalidated
    // by the next call to save() or restore().
    SkDevice* GetTopDevice(const SkCanvas& canvas);

    // Creates a canvas with raster bitmap backing.
    // Set is_opaque if you are going to erase the bitmap and not use
    // transparency: this will enable some optimizations.
    SkCanvas* CreateBitmapCanvas(int width, int height, bool is_opaque);

    // Non-crashing version of CreateBitmapCanvas
    // returns NULL if allocation fails for any reason.
    // Use this instead of CreateBitmapCanvas in places that are likely to
    // attempt to allocate very large canvases (therefore likely to fail),
    // and where it is possible to recover gracefully from the failed allocation.
    SkCanvas* TryCreateBitmapCanvas(int width, int height, bool is_opaque);

    // Returns true if native platform routines can be used to draw on the
    // given canvas. If this function returns false, BeginPlatformPaint will
    // return NULL PlatformSurface.
    bool SupportsPlatformPaint(const SkCanvas* canvas);

    // Draws into the a native platform surface, |context|.  Forwards to
    // DrawToNativeContext on a PlatformDevice instance bound to the top device.
    // If no PlatformDevice instance is bound, is a no-operation.
    void DrawToNativeContext(SkCanvas* canvas, HDC context,
        int x, int y, const RECT* src_rect);

    // Sets the opacity of each pixel in the specified region to be opaque.
    void MakeOpaque(SkCanvas* canvas, int x, int y, int width, int height);

    // These calls should surround calls to platform drawing routines, the
    // surface returned here can be used with the native platform routines.
    //
    // Call EndPlatformPaint when you are done and want to use skia operations
    // after calling the platform-specific BeginPlatformPaint; this will
    // synchronize the bitmap to OS if necessary.
    HDC BeginPlatformPaint(SkCanvas* canvas);
    void EndPlatformPaint(SkCanvas* canvas);

    // Helper class for pairing calls to BeginPlatformPaint and EndPlatformPaint.
    // Upon construction invokes BeginPlatformPaint, and upon destruction invokes
    // EndPlatformPaint.
    class ScopedPlatformPaint
    {
    public:
        explicit ScopedPlatformPaint(SkCanvas* canvas) : canvas_(canvas)
        {
            platform_surface_ = BeginPlatformPaint(canvas);
        }
        ~ScopedPlatformPaint() { EndPlatformPaint(canvas_); }

        // Returns the PlatformSurface to use for native platform drawing calls.
        HDC GetPlatformSurface() { return platform_surface_; }

    private:
        SkCanvas* canvas_;
        HDC platform_surface_;

        // Disallow copy and assign
        ScopedPlatformPaint(const ScopedPlatformPaint&);
        ScopedPlatformPaint& operator=(const ScopedPlatformPaint&);
    };

} //namespace skia

#endif //__skia_platform_canvas_h__