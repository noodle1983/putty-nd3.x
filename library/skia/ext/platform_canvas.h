
#ifndef __skia_ext_platform_canvas_h__
#define __skia_ext_platform_canvas_h__

#pragma once

#include "platform_device_win.h"
#include "SkCanvas.h"

namespace skia
{

    // PlatformCanvas是一个特殊化的规则SkCanvas, 用于配合PlatformDevice来管理平台
    // 相关的绘图. 它同时允许Skia操作和平台相关操作.
    class PlatformCanvas : public SkCanvas
    {
    public:
        // 如果使用无参数版本的构造函数, 必须手动调用initialize().
        PlatformCanvas();
        explicit PlatformCanvas(SkDeviceFactory* factory);
        // 如果想要擦除位图, 且不需要透明度, 可以设置is_opaque: 这么做会有一些
        // 优化.
        PlatformCanvas(int width, int height, bool is_opaque);
        virtual ~PlatformCanvas();

        // 传递shared_section给BitmapPlatformDevice::create.
        PlatformCanvas(int width, int height, bool is_opaque, HANDLE shared_section);

        // 如果你使用的是上面无参数的构造函数, 这是第二步初始化工作.
        bool initialize(int width, int height, bool is_opaque,
            HANDLE shared_section=NULL);

        // 这两个函数应该把平台绘图操作包围起来, 返回的表面可用于本地平台操作.
        //
        // 在调用平台相关的beginPlatformPaint之后, 当你完成绘图想要使用Skia操作时,
        // 需要调用endPlatformPaint; 它会在必要时同步位图到OS.
        PlatformDevice::PlatformSurface beginPlatformPaint();
        void endPlatformPaint();

        // 返回最顶层矩形的平台设备指针, 带有非空的裁剪区. 实际上, 一般要么是顶层
        // 要么什么都没有, 因为通常创建新层时都会设置裁剪区.
        //
        // 如果所有的层都被裁减掉, 会返回一个无用的设备, 这样调用者不必检查返回值.
        // 如果你很关心性能, 在任何绘图操作前检查裁剪区.
        //
        // 跟SkCanvas的getDevice有区别, 因为SkCanvas返回的是最底层的设备.
        //
        // 危险: 不要保存返回的设备, 它会在下一次调用save()或者restore()时失效.
        PlatformDevice& getTopPlatformDevice() const;

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

} //namespace skia

#endif //__skia_ext_platform_canvas_h__