
#ifndef __base_process_h__
#define __base_process_h__

#pragma once

#include <windows.h>

namespace base
{

    // ProcessHandle is a platform specific type which represents the underlying OS
    // handle to a process.
    // ProcessId is a number which identifies the process in the OS.
    typedef HANDLE ProcessHandle;
    typedef DWORD ProcessId;
    typedef HANDLE UserTokenHandle;
    const ProcessHandle kNullProcessHandle = NULL;
    const ProcessId kNullProcessId = 0;

    class Process
    {
    public:
        Process() : process_(kNullProcessHandle) {}

        explicit Process(ProcessHandle handle) : process_(handle) {}

        // A handle to the current process.
        static Process Current();

        // Get/Set the handle for this process. The handle will be 0 if the process
        // is no longer running.
        ProcessHandle handle() const { return process_; }
        void set_handle(ProcessHandle handle)
        {
            process_ = handle;
        }

        // Get the PID for this process.
        ProcessId pid() const;

        // Is the this process the current process.
        bool is_current() const;

        // Close the process handle. This will not terminate the process.
        void Close();

        // Terminates the process with extreme prejudice. The given result code will
        // be the exit code of the process. If the process has already exited, this
        // will do nothing.
        void Terminate(int result_code);

        // A process is backgrounded when it's priority is lower than normal.
        // Return true if this process is backgrounded, false otherwise.
        bool IsProcessBackgrounded() const;

        // Set a process as backgrounded. If value is true, the priority
        // of the process will be lowered. If value is false, the priority
        // of the process will be made "normal" - equivalent to default
        // process priority.
        // Returns true if the priority was changed, false otherwise.
        bool SetProcessBackgrounded(bool value);

        // Returns an integer representing the priority of a process. The meaning
        // of this value is OS dependent.
        int GetPriority() const;

    private:
        ProcessHandle process_;
    };

} //namespace base

#endif //__base_process_h__