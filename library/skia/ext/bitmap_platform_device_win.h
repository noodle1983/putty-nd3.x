
#ifndef __skia_ext_bitmap_platform_device_win_h__
#define __skia_ext_bitmap_platform_device_win_h__

#pragma once

#include "platform_device_win.h"

namespace skia
{

    class BitmapPlatformDeviceFactory : public SkDeviceFactory
    {
    public:
        virtual SkDevice* newDevice(SkCanvas* ignored,
            SkBitmap::Config config, 
            int width, int height,
            bool isOpaque, bool isForLayer);
    };

    // BitmapPlatformDevice是SkBitmap的基本封装, 为SkCanvas提供绘图表面. 设备为
    // Windows提供了一个可写的表面. BitmapPlatformDevice使用CreateDIBSection()创
    // 建一个Skia支持的位图, 这样就可以绘制ClearType文字. 像素数据在设备的位图中,
    // 这样可以被共享.
    //
    // 设备拥有像素数据, 当设备销毁时, 像素数据也变成无效的. 这一点不同于SKIA,
    // 它的像素数据使用了引用计数. 在Skia中, 你可以用另外一个位图这个设备的位图
    // 赋值, 这是允许的. 对于我们, 替换的位图会再设备非法的时候也变成不合法的,
    // 这常常导致一些隐晦的问题. 所以, 不要用其它的位图给这个设备的像素数据赋值,
    // 确保使用的是拷贝.
    class BitmapPlatformDevice : public PlatformDevice
    {
    public:
        // 工程函数. screen_dc用于创建位图, 不会在函数中存储. 如果调用者明确知道位图
        // 完全不透明且允许可以设置is_opaque为true, 这样函数会做一些优化.
        //
        // shared_section参数是可选的(传递NULL使用缺省行为). 如果shared_section非空,
        // 它必须是一个CreateFileMapping返回的文件映射对象. 细节参见CreateDIBSection.
        static BitmapPlatformDevice* create(HDC screen_dc,
            int width, int height, bool is_opaque, HANDLE shared_section);

        // 和上面一样, 只是函数自身获取screen_dc.
        static BitmapPlatformDevice* create(int width, int height,
            bool is_opaque, HANDLE shared_section);

        // 拷贝构造函数. 拷贝时, 复制设备的内部数据指针, 所以对象间共享数据. 这是
        // 因为内部的实现是重量级的(大量的内存和一些GDI对象). 如果一个设备被拷贝,
        // 裁剪区和其它状态都会保持同步.
        //
        // 这意味着复制一个设备并赋值给一个画布是不行的, 因为两个画布都会各自设置
        // 自己的裁剪区, 最终的GDI裁剪矩形也是不一样的.
        //
        // 拷贝和赋值构造被设计用来节省设备或是传递到其它想要直接处理位图数据的函
        // 数.
        BitmapPlatformDevice(const BitmapPlatformDevice& other);
        virtual ~BitmapPlatformDevice();

        // 参见上面关于拷贝构造函数的警告.
        BitmapPlatformDevice& operator=(const BitmapPlatformDevice& other);

        virtual SkDeviceFactory* getDeviceFactory()
        {
            return SkNEW(BitmapPlatformDeviceFactory);
        }

        // 返回位图数据的内存DC. 位图DC是懒创建的.
        virtual HDC getBitmapDC();

        // 加载给定的变化和裁剪区到HDC. 重载SkDevice的.
        virtual void setMatrixClip(const SkMatrix& transform,
            const SkRegion& region,
            const SkClipStack&);

        virtual void drawToHDC(HDC dc, int x, int y, const RECT* src_rect);
        virtual void makeOpaque(int x, int y, int width, int height);
        virtual bool IsVectorial() { return false; }

        // 返回给定位置的颜色值. 不考虑设备上设置的任何变化.
        SkColor getColorAt(int x, int y);

    protected:
        // 刷新Windows DC, 以便Skia能够直接访问像素数据. 重载SkDevice的, 在Skia开始
        // 访问像素数据时调用.
        virtual void onAccessBitmap(SkBitmap* bitmap);

    private:
        // 引用计数数据, 能被多个设备间共享, 保证拷贝构造和赋值构造正常工作. 基类设备
        // 使用的位图已经是引用计数的, 支持拷贝.
        class BitmapPlatformDeviceData;

        // 私有构造函数. 数据应该是引用过的.
        BitmapPlatformDevice(BitmapPlatformDeviceData* data,
            const SkBitmap& bitmap);

        // 设备关联的数据, 保证非空. 我们存储对象的引用.
        BitmapPlatformDeviceData* data_;
    };

} //namespace skia

#endif //__skia_ext_bitmap_platform_device_win_h__