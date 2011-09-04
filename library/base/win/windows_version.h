
#ifndef __base_windows_version_h__
#define __base_windows_version_h__

#pragma once

#include "base/basic_types.h"

typedef void* HANDLE;

namespace base
{
    namespace win
    {

        // 注意: 保持枚举顺序以便调用者可以:
        // "if(base::win::GetVersion() >= base::win::VERSION_VISTA) ...".
        enum Version
        {
            VERSION_PRE_XP = 0, // 不支持.
            VERSION_XP,
            VERSION_SERVER_2003, // Also includes Windows XP Professional x64.
            VERSION_VISTA,
            VERSION_SERVER_2008,
            VERSION_WIN7,
        };

        // A singleton that can be used to query various pieces of information about the
        // OS and process state. Note that this doesn't use the base Singleton class, so
        // it can be used without an AtExitManager.
        class OSInfo
        {
        public:
            struct VersionNumber
            {
                int major;
                int minor;
                int build;
            };

            struct ServicePack
            {
                int major;
                int minor;
            };

            // Windows本地使用的处理器架构.
            // 比如对于x64兼容的处理器, 有以下三种可能:
            //   32程序运行在32位的Windows:                 X86_ARCHITECTURE
            //   32程序通过WOW64运行在64位的Windows:        X64_ARCHITECTURE
            //   64程序运行在64位的Windows:                 X64_ARCHITECTURE
            enum WindowsArchitecture
            {
                X86_ARCHITECTURE,
                X64_ARCHITECTURE,
                IA64_ARCHITECTURE,
                OTHER_ARCHITECTURE,
            };

            // 处理器是否运行在WOW64(64位版本的Windows模拟32位处理器)中. 对于"32位程序
            // 在32位的Windows中执行"和"64位程序在64位的Windows中执行"均返回WOW64_DISABLED.
            // WOW64_UNKNOWN表示"有错误发生", 比如进程的权限不够.
            enum WOW64Status
            {
                WOW64_DISABLED,
                WOW64_ENABLED,
                WOW64_UNKNOWN,
            };

            static OSInfo* GetInstance();

            Version version() const { return version_; }
            // The next two functions return arrays of values, [major, minor(, build)].
            VersionNumber version_number() const { return version_number_; }
            ServicePack service_pack() const { return service_pack_; }
            WindowsArchitecture architecture() const { return architecture_; }
            int processors() const { return processors_; }
            size_t allocation_granularity() const { return allocation_granularity_; }
            WOW64Status wow64_status() const { return wow64_status_; }

            // 和wow64_status()类似, 判断指定进程状态而不是当前进程.
            // 不是成员函数, 所以不需要访问单件实例.
            static WOW64Status GetWOW64StatusForProcess(HANDLE process_handle);

        private:
            OSInfo();
            ~OSInfo();

            Version version_;
            VersionNumber version_number_;
            ServicePack service_pack_;
            WindowsArchitecture architecture_;
            int processors_;
            size_t allocation_granularity_;
            WOW64Status wow64_status_;

            DISALLOW_COPY_AND_ASSIGN(OSInfo);
        };

        // 返回当前运行的Windows版本. 由于使用的很频繁, 封装单件的访问,
        // 实现一个全局函数.
        Version GetVersion();

    } //namespace win
} //namespace base

#endif //__base_windows_version_h__