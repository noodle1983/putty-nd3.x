
#ifndef __base_paths_h__
#define __base_paths_h__

#pragma once

namespace base
{

    enum
    {
        PATH_WIN_START = 100,

        DIR_WINDOWS,            // Windows directory, usually "c:\windows"
        DIR_SYSTEM,             // Usually c:\windows\system32"
        DIR_PROGRAM_FILES,      // Usually c:\program files
        DIR_PROGRAM_FILESX86,   // Usually c:\program files or c:\program files (x86)

        DIR_IE_INTERNET_CACHE,  // Temporary Internet Files directory.
        DIR_COMMON_START_MENU,  // Usually "C:\Documents and Settings\All Users\
                                // Start Menu\Programs"
        DIR_START_MENU,         // Usually "C:\Documents and Settings\<user>\
                                // Start Menu\Programs"
        DIR_APP_DATA,           // Application Data directory under the user profile.
        DIR_PROFILE,            // Usually "C:\Documents and settings\<user>.
        DIR_LOCAL_APP_DATA_LOW, // Local AppData directory for low integrity level.
        DIR_LOCAL_APP_DATA,     // "Local Settings\Application Data" directory under the
                                // user profile.
        PATH_WIN_END
    };

    enum BasePathKey
    {
        PATH_START = 0,

        DIR_CURRENT,    // current directory
        DIR_EXE,        // directory containing FILE_EXE
        DIR_MODULE,     // directory containing FILE_MODULE
        DIR_TEMP,       // temporary directory
        FILE_EXE,       // Path and filename of the current executable.
        FILE_MODULE,    // Path and filename of the module containing the code for the
                        // PathService (which could differ from FILE_EXE if the
                        // PathService were compiled into a shared object, for example).
        DIR_SOURCE_ROOT,// Returns the root of the source tree.  This key is useful
                        // for tests that need to locate various resources.  It
                        // should not be used outside of test code.
        PATH_END
    };

} //namespace base

#endif //__base_paths_h__