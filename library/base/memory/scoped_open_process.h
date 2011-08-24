
#ifndef __base_scoped_open_process_h__
#define __base_scoped_open_process_h__

#pragma once

#include "base/process.h"
#include "base/process_util.h"

namespace base
{

    // A class that opens a process from its process id and closes it when the
    // instance goes out of scope.
    class ScopedOpenProcess
    {
    public:
        ScopedOpenProcess() : handle_(kNullProcessHandle) {}

        // Automatically close the process.
        ~ScopedOpenProcess()
        {
            Close();
        }

        // Open a new process by pid. Closes any previously opened process (even if
        // opening the new one fails).
        bool Open(ProcessId pid)
        {
            Close();
            return OpenProcessHandle(pid, &handle_);
        }

        // Close the previously opened process.
        void Close()
        {
            if(handle_ == kNullProcessHandle)
            {
                return;
            }

            CloseProcessHandle(handle_);
            handle_ = kNullProcessHandle;
        }

        ProcessHandle handle() const { return handle_; }

    private:
        ProcessHandle handle_;
        DISALLOW_COPY_AND_ASSIGN(ScopedOpenProcess);
    };

} //namespace base

#endif //__base_scoped_open_process_h__