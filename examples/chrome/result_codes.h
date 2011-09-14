
#ifndef __result_codes_h__
#define __result_codes_h__

#pragma once

#include "base/process_util.h"

// This file consolidates all the return codes for the browser and renderer
// process. The return code is the value that:
// a) is returned by main() or winmain(), or
// b) specified in the call for ExitProcess() or TerminateProcess(), or
// c) the exception value that causes a process to terminate.
//
// It is advisable to not use negative numbers because the Windows API returns
// it as an unsigned long and the exception values have high numbers. For
// example EXCEPTION_ACCESS_VIOLATION value is 0xC0000005.

namespace content
{

    enum ResultCode
    {
        // Process terminated normally.
        RESULT_CODE_NORMAL_EXIT = 0,

        // Process was killed by user or system.
        RESULT_CODE_KILLED = 1,

        // Process hung.
        RESULT_CODE_HUNG = 2,

        // A bad message caused the process termination.
        RESULT_CODE_KILLED_BAD_MESSAGE,

        // Last return code (keep this last).
        RESULT_CODE_LAST_CODE
    };

} //namespace content

#endif //__result_codes_h__