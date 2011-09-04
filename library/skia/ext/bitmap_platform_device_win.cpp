
#include "bitmap_platform_device_win.h"

#include <windows.h>

#include "SkMatrix.h"
#include "SkRefCnt.h"
#include "SkRegion.h"
#include "SkUtils.h"

#include "bitmap_platform_device_data.h"

namespace skia
{

    BitmapPlatformDevice::BitmapPlatformDeviceData::BitmapPlatformDeviceData(
        HBITMAP hbitmap) : bitmap_context_(hbitmap), hdc_(NULL),
        config_dirty_(true) // 想要在下一次加载配置.
    {
        // 初始化裁剪区为整个位图.
        BITMAP bitmap_data;
        if(GetObject(bitmap_context_, sizeof(BITMAP), &bitmap_data))
        {
            SkIRect rect;
            rect.set(0, 0, bitmap_data.bmWidth, bitmap_data.bmHeight);
            clip_region_ = SkRegion(rect);
        }

        transform_.reset();
    }

    BitmapPlatformDevice::BitmapPlatformDeviceData::~BitmapPlatformDeviceData()
    {
        if(hdc_)
        {
            ReleaseBitmapDC();
        }

        // 同时清理位图数据和位图句柄.
        DeleteObject(bitmap_context_);
    }

    HDC BitmapPlatformDevice::BitmapPlatformDeviceData::GetBitmapDC()
    {
        if(!hdc_)
        {
            hdc_ = CreateCompatibleDC(NULL);
            InitializeDC(hdc_);
            HGDIOBJ old_bitmap = SelectObject(hdc_, bitmap_context_);
            // 内存DC刚创建时, 显示表面为1*1像素的黑白位图. 所以我们选入自己的位图,
            // 在删除前必须先选出.
            DeleteObject(old_bitmap);
        }

        LoadConfig();
        return hdc_;
    }

    void BitmapPlatformDevice::BitmapPlatformDeviceData::ReleaseBitmapDC()
    {
        SkASSERT(hdc_);
        DeleteDC(hdc_);
        hdc_ = NULL;
    }

    bool BitmapPlatformDevice::BitmapPlatformDeviceData::IsBitmapDCCreated() const
    {
        return hdc_ != NULL;
    }

    void BitmapPlatformDevice::BitmapPlatformDeviceData::SetMatrixClip(
        const SkMatrix& transform, const SkRegion& region)
    {
        transform_ = transform;
        clip_region_ = region;
        config_dirty_ = true;
    }

    void BitmapPlatformDevice::BitmapPlatformDeviceData::LoadConfig()
    {
        if(!config_dirty_ || !hdc_)
        {
            return; // 什么都不做.
        }
        config_dirty_ = false;

        // 变换.
        LoadTransformToDC(hdc_, transform_);
        LoadClippingRegionToDC(hdc_, clip_region_, transform_);
    }

    // 使用静态工厂函数替代普通的构造函数, 这样可以在调用构造前创建像素数据.
    // 之所以要这么做是因为调用基类构造函数时需要像素数据.
    BitmapPlatformDevice* BitmapPlatformDevice::create(HDC screen_dc,
        int width, int height, bool is_opaque, HANDLE shared_section)
    {
        SkBitmap bitmap;

        // CreateDIBSection不支持创建空位图, 所以只能创建一个最小的.
        if((width==0) || (height==0))
        {
            width = 1;
            height = 1;
        }

        BITMAPINFOHEADER hdr = { 0 };
        hdr.biSize = sizeof(BITMAPINFOHEADER);
        hdr.biWidth = width;
        hdr.biHeight = -height; // 负号表示自上而下的位图.
        hdr.biPlanes = 1;
        hdr.biBitCount = 32;
        hdr.biCompression = BI_RGB; // 不压缩.
        hdr.biSizeImage = 0;
        hdr.biXPelsPerMeter = 1;
        hdr.biYPelsPerMeter = 1;
        hdr.biClrUsed = 0;
        hdr.biClrImportant = 0;

        void* data = NULL;
        HBITMAP hbitmap = CreateDIBSection(screen_dc,
            reinterpret_cast<BITMAPINFO*>(&hdr), 0,
            &data, shared_section, 0);
        if(!hbitmap)
        {
            return NULL;
        }

        bitmap.setConfig(SkBitmap::kARGB_8888_Config, width, height);
        bitmap.setPixels(data);
        bitmap.setIsOpaque(is_opaque);

        // 如果数据是传入的, 不要擦除它!
        if(!shared_section)
        {
            if(is_opaque)
            {
#ifndef NDEBUG
                // 为了辅助查找bugs, 设置背景色为某种很容易被发现的颜色.
                bitmap.eraseARGB(255, 0, 255, 128); // 亮蓝绿.
#endif
            }
            else
            {
                bitmap.eraseARGB(0, 0, 0, 0);
            }
        }

        // 设备对象接管HBITMAP的所有权. 数据对象的初始引用计数为1, 符合构造
        // 函数要求.
        return new BitmapPlatformDevice(new BitmapPlatformDeviceData(hbitmap),
            bitmap);
    }

    // static
    BitmapPlatformDevice* BitmapPlatformDevice::create(int width,
        int height, bool is_opaque, HANDLE shared_section)
    {
        HDC screen_dc = GetDC(NULL);
        BitmapPlatformDevice* device = BitmapPlatformDevice::create(
            screen_dc, width, height, is_opaque, shared_section);
        ReleaseDC(NULL, screen_dc);
        return device;
    }

    // 设备拥有HBITMAP, 同时也拥有里面的像素数据, 不会转移所有权给SkDevice的位图.
    BitmapPlatformDevice::BitmapPlatformDevice(BitmapPlatformDeviceData* data,
        const SkBitmap& bitmap) : SkDevice(bitmap), data_(data)
    {
        // 数据对象已经在create()中引用过.
        SetPlatformDevice(this, this);
    }

    BitmapPlatformDevice::~BitmapPlatformDevice()
    {
        data_->unref();
    }

    HDC BitmapPlatformDevice::BeginPlatformPaint()
    {
        return data_->GetBitmapDC();
    }

    void BitmapPlatformDevice::EndPlatformPaint()
    {
        PlatformDevice::EndPlatformPaint();
    }

    void BitmapPlatformDevice::DrawToNativeContext(HDC dc, int x, int y,
        const RECT* src_rect)
    {
        bool created_dc = !data_->IsBitmapDCCreated();
        HDC source_dc = BeginPlatformPaint();

        RECT temp_rect;
        if(!src_rect)
        {
            temp_rect.left = 0;
            temp_rect.right = width();
            temp_rect.top = 0;
            temp_rect.bottom = height();
            src_rect = &temp_rect;
        }

        int copy_width = src_rect->right - src_rect->left;
        int copy_height = src_rect->bottom - src_rect->top;

        // 我们需要对位图重置变换, 否则(0, 0)将不再是左上角.
        SkMatrix identity;
        identity.reset();

        LoadTransformToDC(source_dc, identity);
        if(isOpaque())
        {
            BitBlt(dc,
                x,
                y,
                copy_width,
                copy_height,
                source_dc,
                src_rect->left,
                src_rect->top,
                SRCCOPY);
        }
        else
        {
            SkASSERT(copy_width!=0 && copy_height!=0);
            BLENDFUNCTION blend_function = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            GdiAlphaBlend(dc,
                x,
                y,
                copy_width,
                copy_height,
                source_dc,
                src_rect->left,
                src_rect->top,
                copy_width,
                copy_height,
                blend_function);
        }
        LoadTransformToDC(source_dc, data_->transform());

        EndPlatformPaint();
        if(created_dc)
        {
            data_->ReleaseBitmapDC();
        }
    }

    void BitmapPlatformDevice::setMatrixClip(const SkMatrix& transform,
        const SkRegion& region, const SkClipStack&)
    {
        data_->SetMatrixClip(transform, region);
    }

    void BitmapPlatformDevice::onAccessBitmap(SkBitmap* bitmap)
    {
        // 优化: 我们只应该在DC发生了GDI操作时才调用flush.
        if(data_->IsBitmapDCCreated())
        {
            GdiFlush();
        }
    }

    SkDevice* BitmapPlatformDevice::onCreateCompatibleDevice(
        SkBitmap::Config config, int width, int height, bool isOpaque)
    {
        SkASSERT(config == SkBitmap::kARGB_8888_Config);
        return BitmapPlatformDevice::create(width, height, isOpaque, NULL);
    }

} //namespace skia