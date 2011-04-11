
#include "vector_canvas.h"

#include "vector_platform_device_win.h"

namespace skia
{

    VectorCanvas::VectorCanvas()
        : PlatformCanvas(SkNEW(VectorPlatformDeviceFactory)) {}

    VectorCanvas::VectorCanvas(SkDeviceFactory* factory)
        : PlatformCanvas(factory) {}

    VectorCanvas::VectorCanvas(HDC dc, int width, int height)
    {
        bool initialized = initialize(dc, width, height);
        if(!initialized)
        {
            __debugbreak();
        }
    }

    VectorCanvas::~VectorCanvas() {}

    bool VectorCanvas::initialize(HDC context, int width, int height)
    {
        SkDevice* device = VectorPlatformDeviceFactory::CreateDevice(
            width, height, true, context);
        if(!device)
        {
            return false;
        }

        setDevice(device);
        device->unref(); // 创建后的引用计数为1, setDevice会再引用一次.
        return true;
    }

    SkBounder* VectorCanvas::setBounder(SkBounder* bounder)
    {
        if(!IsTopDeviceVectorial())
        {
            return PlatformCanvas::setBounder(bounder);
        }

        SkASSERT(false);
        return NULL;
    }

    SkDrawFilter* VectorCanvas::setDrawFilter(SkDrawFilter* filter)
    {
        SkASSERT(false);
        return NULL;
    }

    bool VectorCanvas::IsTopDeviceVectorial() const
    {
        return getTopPlatformDevice().IsVectorial();
    }

} //namespace skia