
#ifndef __base_win_windows_version_h__
#define __base_win_windows_version_h__

#pragma once

typedef void* HANDLE;

namespace base
{

    // 注意: 保持枚举顺序以便调用者可以"if(GetWinVersion() > WINVERSION_2000) ...".
    // 枚举值是可以修改的.
    enum WinVersion
    {
        WINVERSION_PRE_2000 = 0, // 不支持.
        WINVERSION_2000 = 1, // 不支持.
        WINVERSION_XP = 2,
        WINVERSION_SERVER_2003 = 3,
        WINVERSION_VISTA = 4,
        WINVERSION_2008 = 5,
        WINVERSION_WIN7 = 6,
    };

    // 返回当前运行的Windows版本.
    WinVersion GetWinVersion();

    // 返回安装的SP包的主要和次要版本号.
    void GetServicePackLevel(int* major, int* minor);

    enum WindowsArchitecture
    {
        X86_ARCHITECTURE,
        X64_ARCHITECTURE,
        IA64_ARCHITECTURE,
        OTHER_ARCHITECTURE,
    };

    // 返回Windows本地使用的处理器架构.
    // 比如对于x64兼容的处理器, 有以下三种可能:
    //   32程序运行在32位的Windows:                 X86_ARCHITECTURE
    //   32程序通过WOW64运行在64位的Windows:        X64_ARCHITECTURE
    //   64程序运行在64位的Windows:                 X64_ARCHITECTURE
    WindowsArchitecture GetWindowsArchitecture();

    enum WOW64Status
    {
        WOW64_DISABLED,
        WOW64_ENABLED,
        WOW64_UNKNOWN,
    };

    // 返回处理器是否运行在WOW64(64位版本的Windows模拟32位处理器)中. 对于"32位程序
    // 在32位的Windows中执行"和"64位程序在64位的Windows中执行"均返回WOW64_DISABLED.
    // WOW64_UNKNOWN表示"有错误发生", 比如进程的权限不够.
    WOW64Status GetWOW64Status();

    // 和GetWOW64Status()类似, 判断指定进程状态而不是当前进程.
    WOW64Status GetWOW64StatusForProcess(HANDLE process_handle);

} //namespace base

#endif //__base_win_windows_version_h__