
#ifndef __skia_platform_device_win_h__
#define __skia_platform_device_win_h__

#pragma once

#include <windows.h>

#include <vector>

#include "SkDevice.h"

class SkMatrix;
class SkPath;
class SkRegion;

namespace skia
{

    class PlatformDevice;

    // The following routines provide accessor points for the functionality
    // exported by the various PlatformDevice ports.  The PlatformDevice, and
    // BitmapPlatformDevice classes inherit directly from SkDevice, which is no
    // longer a supported usage-pattern for skia.  In preparation of the removal of
    // these classes, all calls to PlatformDevice::* should be routed through these
    // helper functions.

    // Bind a PlatformDevice instance, |platform_device| to |device|.  Subsequent
    // calls to the functions exported below will forward the request to the
    // corresponding method on the bound PlatformDevice instance.    If no
    // PlatformDevice has been bound to the SkDevice passed, then the routines are
    // NOPS.
    void SetPlatformDevice(SkDevice* device, PlatformDevice* platform_device);
    PlatformDevice* GetPlatformDevice(SkDevice* device);

    // Initializes the default settings and colors in a device context.
    void InitializeDC(HDC context);

    // PlatformDevice是SkBitmap的基本封装, 为SkCanvas提供绘图表面. 设备为Windows提供
    // 了一个可写的表面, 且提供了与GDI绘图函数配合的功能. PlatformDevice是抽象类必须
    // 被派生实现, 要么使用后台位图, 要么不用.
    class PlatformDevice : public SkDevice
    {
    public:
        // The DC that corresponds to the bitmap, used for GDI operations drawing
        // into the bitmap. This is possibly heavyweight, so it should be existant
        // only during one pass of rendering.
        virtual HDC BeginPlatformPaint() = 0;

        // Finish a previous call to beginPlatformPaint.
        virtual void EndPlatformPaint();

        // Draws to the given screen DC, if the bitmap DC doesn't exist, this will
        // temporarily create it. However, if you have created the bitmap DC, it will
        // be more efficient if you don't free it until after this call so it doesn't
        // have to be created twice.  If src_rect is null, then the entirety of the
        // source device will be copied.
        virtual void DrawToNativeContext(HDC dc, int x, int y,
            const RECT* src_rect) = 0;

        // Sets the opacity of each pixel in the specified region to be opaque.
        virtual void MakeOpaque(int x, int y, int width, int height) {}

        // Returns if GDI is allowed to render text to this device.
        virtual bool IsNativeFontRenderingAllowed() { return true; }

        // True if AlphaBlend() was called during a
        // BeginPlatformPaint()/EndPlatformPaint() pair.
        // Used by the printing subclasses.  See |VectorPlatformDeviceEmf|.
        virtual bool AlphaBlendUsed() const { return false; }

        // Loads a SkPath into the GDI context. The path can there after be used for
        // clipping or as a stroke. Returns false if the path failed to be loaded.
        static bool LoadPathToDC(HDC context, const SkPath& path);

        // Loads a SkRegion into the GDI context.
        static void LoadClippingRegionToDC(HDC context, const SkRegion& region,
            const SkMatrix& transformation);

    protected:
        struct CubicPoints
        {
            SkPoint p[4];
        };
        typedef std::vector<CubicPoints> CubicPath;
        typedef std::vector<CubicPath> CubicPaths;

        // 传递|bitmap|到SkDevice的构造函数.
        PlatformDevice(const SkBitmap& bitmap);

        // 加载Skia的变换到DC的世界变换, 不包括视角(GDI不支持).
        static void LoadTransformToDC(HDC dc, const SkMatrix& matrix);

        // SkPath的路径转换成一系列的三次贝塞尔曲线路径.
        static bool SkPathToCubicPaths(CubicPaths* paths, const SkPath& skpath);
    };

} //namespace skia

#endif //__skia_platform_device_win_h__