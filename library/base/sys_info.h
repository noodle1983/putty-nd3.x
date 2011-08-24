
#ifndef __base_sys_info_h__
#define __base_sys_info_h__

#pragma once

#include <string>

#include "basic_types.h"

class FilePath;

namespace base
{

    class SysInfo
    {
    public:
        // 返回本机逻辑处理器/核的数量.
        static int NumberOfProcessors();

        // 返回本机物理内存容量.
        static int64 AmountOfPhysicalMemory();

        // 返回本机物理内存容量(单位M).
        static int AmountOfPhysicalMemoryMB()
        {
            return static_cast<int>(AmountOfPhysicalMemory() / 1024 / 1024);
        }

        // 获取|path|所在磁盘的剩余空间字节数, 失败返回-1.
        static int64 AmountOfFreeDiskSpace(const FilePath& path);

        // 返回操作系统的名字.
        static std::string OperatingSystemName();

        // 返回操作系统的版本.
        static std::string OperatingSystemVersion();

        // 返回操作系统版本的数值.
        static void OperatingSystemVersionNumbers(int32* major_version,
            int32* minor_version, int32* bugfix_version);

        // 返回系统的CPU架构, 精确值在不同平台会有差别.
        static std::string CPUArchitecture();

        // 返回主显示器水平垂直像素数.
        static void GetPrimaryDisplayDimensions(int* width, int* height);

        // 返回显示器个数.
        static int DisplayCount();

        // 返回VM系统分配的最小内存粒度.
        static size_t VMAllocationGranularity();
    };

} //namespace base

#endif //__base_sys_info_h__