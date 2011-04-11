
#include "sys_info.h"

#include <windows.h>

#include "file_path.h"
#include "logging.h"
#include "stringprintf.h"

namespace base
{

    // static
    int SysInfo::NumberOfProcessors()
    {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        return static_cast<int>(info.dwNumberOfProcessors);
    }

    // static
    int64 SysInfo::AmountOfPhysicalMemory()
    {
        MEMORYSTATUSEX memory_info;
        memory_info.dwLength = sizeof(memory_info);
        if(!GlobalMemoryStatusEx(&memory_info))
        {
            NOTREACHED();
            return 0;
        }

        int64 rv = static_cast<int64>(memory_info.ullTotalPhys);
        if(rv < 0)
        {
            rv = kint64max;
        }
        return rv;
    }

    // static
    int64 SysInfo::AmountOfFreeDiskSpace(const FilePath& path)
    {
        ULARGE_INTEGER available, total, free;
        if(!GetDiskFreeSpaceExW(path.value().c_str(), &available, &total, &free))
        {
            return -1;
        }
        int64 rv = static_cast<int64>(available.QuadPart);
        if(rv < 0)
        {
            rv = kint64max;
        }
        return rv;
    }

    // static
    std::string SysInfo::OperatingSystemName()
    {
        return "Windows NT";
    }

    // static
    std::string SysInfo::OperatingSystemVersion()
    {
        OSVERSIONINFO info = { 0 };
        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&info);

        return base::StringPrintf("%lu.%lu",
            info.dwMajorVersion, info.dwMinorVersion);
    }

    // static
    std::string SysInfo::CPUArchitecture()
    {
        // TODO: 支持不同架构的时候会变化.
        return "x86";
    }

    // static
    void SysInfo::GetPrimaryDisplayDimensions(int* width, int* height)
    {
        if(width)
        {
            *width = GetSystemMetrics(SM_CXSCREEN);
        }

        if(height)
        {
            *height = GetSystemMetrics(SM_CYSCREEN);
        }
    }

    // static
    int SysInfo::DisplayCount()
    {
        return GetSystemMetrics(SM_CMONITORS);
    }

    // static
    size_t SysInfo::VMAllocationGranularity()
    {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);

        return sysinfo.dwAllocationGranularity;
    }

    // static
    void SysInfo::OperatingSystemVersionNumbers(int32 *major_version,
        int32* minor_version, int32* bugfix_version)
    {
        OSVERSIONINFO info = { 0 };
        info.dwOSVersionInfoSize = sizeof(info);
        GetVersionEx(&info);
        *major_version = info.dwMajorVersion;
        *minor_version = info.dwMinorVersion;
        *bugfix_version = 0;
    }

} //namespace base