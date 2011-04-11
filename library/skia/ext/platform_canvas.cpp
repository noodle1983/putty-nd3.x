
#include "platform_canvas.h"

#include "bitmap_platform_device_win.h"

namespace skia
{

    PlatformCanvas::PlatformCanvas()
        : SkCanvas(SkNEW(BitmapPlatformDeviceFactory)) {}

    PlatformCanvas::PlatformCanvas(SkDeviceFactory* factory)
        : SkCanvas(factory) {}

    SkDevice* PlatformCanvas::setBitmapDevice(const SkBitmap&)
    {
        SkASSERT(false); // 不能调用.
        return NULL;
    }

    PlatformDevice& PlatformCanvas::getTopPlatformDevice() const
    {
        // 我们所有的设备必须是平台相关的.
        SkCanvas::LayerIter iter(const_cast<PlatformCanvas*>(this), false);
        return *static_cast<PlatformDevice*>(iter.device());
    }

    // static
    size_t PlatformCanvas::StrideForWidth(unsigned width)
    {
        return 4 * width;
    }

    bool PlatformCanvas::initializeWithDevice(SkDevice* device)
    {
        if(!device)
        {
            return false;
        }

        setDevice(device);
        device->unref(); // 创建后的引用计数为1, setDevice会再引用一次.
        return true;
    }

} //namespace skia