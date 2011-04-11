
#include "platform_canvas.h"

#include <windows.h>
#include <psapi.h>

#include "bitmap_platform_device_win.h"

#pragma comment(lib, "psapi.lib")

namespace skia
{

    // 崩溃分析时禁止优化.
#pragma optimize("", off)

    // 失败时崩溃.
#define CHECK(condition) if(!(condition)) __debugbreak();

    // 使进程崩溃. 当位图分配失败时调用, 函数尝试确定失败的原因, 并在适当的地方触发
    // 崩溃. 这样我们可以更加清楚导致崩溃的原因. 函数需要传入位图的宽高, 这样可以试
    // 着创建位图定位错误.
    //
    // 注意在沙盒渲染器中, 函数调用GetProcessMemoryInfo()会导致崩溃, 因为此时会加载
    // load psapi.dll, 这么做是正确的, 但这种崩溃有可能让你摸不着头脑.
    void CrashForBitmapAllocationFailure(int w, int h)
    {
        // 存储扩展的错误信息到一个调试时容易找到的地方.
        struct
        {
            unsigned int last_error;
            unsigned int diag_error;
        } extended_error_info;
        extended_error_info.last_error = GetLastError();
        extended_error_info.diag_error = 0;

        // 如果位图太大, 有可能分配失败.
        // 使用64M像素=256MB, 每个像素4字节来检查.
        const __int64 kGinormousBitmapPxl = 64000000;
        CHECK(static_cast<__int64>(w) * static_cast<__int64>(h) <
            kGinormousBitmapPxl);

        // 每个进程GDI对象的上限是10K. 如果接近这个值, 很可能出现问题.
        const unsigned int kLotsOfGDIObjects = 9990;
        unsigned int num_gdi_objects = GetGuiResources(GetCurrentProcess(),
            GR_GDIOBJECTS);
        if(num_gdi_objects == 0)
        {
            extended_error_info.diag_error = GetLastError();
            CHECK(0);
        }
        CHECK(num_gdi_objects < kLotsOfGDIObjects);

        // 如果过度使用虚拟地址空间, 创建位图需要的内存可能不足.
        const SIZE_T kLotsOfMem = 1500000000; // 1.5GB.
        PROCESS_MEMORY_COUNTERS pmc;
        if(!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        {
            extended_error_info.diag_error = GetLastError();
            CHECK(0);
        }
        CHECK(pmc.PagefileUsage < kLotsOfMem);

        // 其它的问题全部在这里崩溃.
        CHECK(0);
    }

    // 使进程崩溃. 跟CrashForBitmapAllocationFailure()不同, 当位图分配失败时尝试
    // 检测是不是无效的共享位图句柄问题.
    void CrashIfInvalidSection(HANDLE shared_section)
    {
        DWORD handle_info = 0;
        CHECK(GetHandleInformation(shared_section, &handle_info) == TRUE);
    }

    // 恢复优化选项.
#pragma optimize("", on)

    PlatformCanvas::PlatformCanvas(int width, int height, bool is_opaque)
        : SkCanvas(SkNEW(BitmapPlatformDeviceFactory))
    {
        bool initialized = initialize(width, height, is_opaque, NULL);
        if(!initialized)
        {
            CrashForBitmapAllocationFailure(width, height);
        }
    }

    PlatformCanvas::PlatformCanvas(int width, int height, bool is_opaque,
        HANDLE shared_section) : SkCanvas(SkNEW(BitmapPlatformDeviceFactory))
    {
        bool initialized = initialize(width, height, is_opaque, shared_section);
        if(!initialized)
        {
            CrashIfInvalidSection(shared_section);
            CrashForBitmapAllocationFailure(width, height);
        }
    }

    PlatformCanvas::~PlatformCanvas() {}

    bool PlatformCanvas::initialize(int width,
        int height,
        bool is_opaque,
        HANDLE shared_section)
    {
        return initializeWithDevice(BitmapPlatformDevice::create(
            width, height, is_opaque, shared_section));
    }

    HDC PlatformCanvas::beginPlatformPaint()
    {
        return getTopPlatformDevice().getBitmapDC();
    }

    void PlatformCanvas::endPlatformPaint()
    {
        // 这里不清理DC, 这样它还可能被再次使用到. 在onAccessBitmap中完成刷新.
    }

} //namespace skia