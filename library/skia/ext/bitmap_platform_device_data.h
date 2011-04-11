
#ifndef __skia_ext_bitmap_platform_device_data_h__
#define __skia_ext_bitmap_platform_device_data_h__

#include "bitmap_platform_device_win.h"

namespace skia
{

    class BitmapPlatformDevice::BitmapPlatformDeviceData : public SkRefCnt
    {
    public:
        typedef HBITMAP PlatformContext;

        explicit BitmapPlatformDeviceData(PlatformContext bitmap);

        // 创建/销毁hdc_, 位图使用的内存DC.
        HDC GetBitmapDC();
        void ReleaseBitmapDC();
        bool IsBitmapDCCreated() const;

        // 设置变换和裁剪操作. 不会直接更新设备环境, 而是设置脏标记. 所有改变将会在
        // 下次调用LoadConfig时生效.
        void SetMatrixClip(const SkMatrix& transform, const SkRegion& region);

        // 加载当前变换和裁剪到设备环境. 即使|bitmap_context_|为空也可以调用(无操作).
        void LoadConfig();

        const SkMatrix& transform() const
        {
            return transform_;
        }

        PlatformContext bitmap_context()
        {
            return bitmap_context_;
        }

    private:
        virtual ~BitmapPlatformDeviceData();

        // 懒创建的图形设备, 用于在位图中绘制.
        PlatformContext bitmap_context_;

        // 懒创建的DC, 用于在位图中绘制. 参见GetBitmapDC().
        HDC hdc_;

        // 当有变换或裁剪没有设置到设备环境时为true. 设备环境在每次文本操作时返回,
        // 变换或裁剪不是每次都会变. 变量可以节省不必要的加载变换和裁剪的时间.
        bool config_dirty_;

        // 设备环境的变换矩阵: 需要单独维护这个变量, 因为设备环境还没创建的时候
        // 可能已经需要更新这个值.
        SkMatrix transform_;

        // 当前裁剪区.
        SkRegion clip_region_;

        // 禁止拷贝和赋值构造函数.
        BitmapPlatformDeviceData(const BitmapPlatformDeviceData&);
        BitmapPlatformDeviceData& operator=(const BitmapPlatformDeviceData&);
    };

} //namespace skia

#endif //__skia_ext_bitmap_platform_device_data_h__