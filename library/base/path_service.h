
#ifndef __base_path_service_h__
#define __base_path_service_h__

#pragma once

class FilePath;

namespace base
{

    enum BasePathKey
    {
        PATH_START = 0,

        DIR_CURRENT,    // 当前目录.
        DIR_EXE,        // 包含FILE_EXE的目录.
        DIR_MODULE,     // 包含FILE_MODULE的目录.
        DIR_TEMP,       // 临时目录.
        FILE_EXE,       // 可执行程序的全路径.
        FILE_MODULE,    // 当前模块的全路径.
        PATH_END
    };

    enum
    {
        PATH_WIN_START = 100,

        DIR_WINDOWS,                // Windows目录 c:\windows
        DIR_SYSTEM,                 // 系统目录 c:\windows\system32
        DIR_PROGRAM_FILES,          // 程序目录 c:\program files

        DIR_IE_INTERNET_CACHE,      // Internet临时文件目录.
        DIR_COMMON_START_MENU,      // 开始菜单目录 C:\Documents and Settings\All Users\
                                    // Start Menu\Programs
        DIR_START_MENU,             // 开始菜单目录 C:\Documents and Settings\<user>\
                                    // Start Menu\Programs
        DIR_COMMON_APP_DATA,        // 应用数据目录 All Users\Application Data
        DIR_APP_DATA,               // 应用数据目录 <user>\Application Data
        DIR_PROFILE,                // 用户目录 C:\Documents and settings\<user>
        DIR_LOCAL_APP_DATA_LOW,     // 低完整性级别应用数据目录
        DIR_LOCAL_APP_DATA,         // 局部应用数据目录 Local Settings\Application Data
        PATH_WIN_END
    };

    bool PathProvider(int key, FilePath* result);

} //namespace base

#endif //__base_path_service_h__