
// This file/namespace contains utility functions for enumerating, ending and
// computing statistics of processes.

#ifndef __base_process_util_h__
#define __base_process_util_h__

#pragma once

#include <windows.h>
#include <tlhelp32.h>

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "basic_types.h"
#include "file_path.h"
#include "process.h"

class CommandLine;

namespace base
{

    struct ProcessEntry : public PROCESSENTRY32
    {
        ProcessId pid() const { return th32ProcessID; }
        ProcessId parent_pid() const { return th32ParentProcessID; }
        const wchar_t* exe_file() const { return szExeFile; }
    };

    struct IoCounters : public IO_COUNTERS {};

    // Process access masks. These constants provide platform-independent
    // definitions for the standard Windows access masks.
    // See http://msdn.microsoft.com/en-us/library/ms684880(VS.85).aspx for
    // the specific semantics of each mask value.
    const uint32 kProcessAccessTerminate              = PROCESS_TERMINATE;
    const uint32 kProcessAccessCreateThread           = PROCESS_CREATE_THREAD;
    const uint32 kProcessAccessSetSessionId           = PROCESS_SET_SESSIONID;
    const uint32 kProcessAccessVMOperation            = PROCESS_VM_OPERATION;
    const uint32 kProcessAccessVMRead                 = PROCESS_VM_READ;
    const uint32 kProcessAccessVMWrite                = PROCESS_VM_WRITE;
    const uint32 kProcessAccessDuplicateHandle        = PROCESS_DUP_HANDLE;
    const uint32 kProcessAccessCreateProcess          = PROCESS_CREATE_PROCESS;
    const uint32 kProcessAccessSetQuota               = PROCESS_SET_QUOTA;
    const uint32 kProcessAccessSetInformation         = PROCESS_SET_INFORMATION;
    const uint32 kProcessAccessQueryInformation       = PROCESS_QUERY_INFORMATION;
    const uint32 kProcessAccessSuspendResume          = PROCESS_SUSPEND_RESUME;
    const uint32 kProcessAccessQueryLimitedInfomation =
        PROCESS_QUERY_LIMITED_INFORMATION;
    const uint32 kProcessAccessWaitForTermination     = SYNCHRONIZE;

    // Return status values from GetTerminationStatus.  Don't use these as
    // exit code arguments to KillProcess*(), use platform/application
    // specific values instead.
    enum TerminationStatus
    {
        TERMINATION_STATUS_NORMAL_TERMINATION,   // zero exit status
        TERMINATION_STATUS_ABNORMAL_TERMINATION, // non-zero exit status
        TERMINATION_STATUS_PROCESS_WAS_KILLED,   // e.g. SIGKILL or task manager kill
        TERMINATION_STATUS_PROCESS_CRASHED,      // e.g. Segmentation fault
        TERMINATION_STATUS_STILL_RUNNING,        // child hasn't exited yet
        TERMINATION_STATUS_MAX_ENUM
    };

    // Returns the id of the current process.
    ProcessId GetCurrentProcId();

    // Returns the ProcessHandle of the current process.
    ProcessHandle GetCurrentProcessHandle();

    // Converts a PID to a process handle. This handle must be closed by
    // CloseProcessHandle when you are done with it. Returns true on success.
    bool OpenProcessHandle(ProcessId pid, ProcessHandle* handle);

    // Converts a PID to a process handle. On Windows the handle is opened
    // with more access rights and must only be used by trusted code.
    // You have to close returned handle using CloseProcessHandle. Returns true
    // on success.
    // TODO(sanjeevr): Replace all calls to OpenPrivilegedProcessHandle with the
    // more specific OpenProcessHandleWithAccess method and delete this.
    bool OpenPrivilegedProcessHandle(ProcessId pid, ProcessHandle* handle);

    // Converts a PID to a process handle using the desired access flags. Use a
    // combination of the kProcessAccess* flags defined above for |access_flags|.
    bool OpenProcessHandleWithAccess(ProcessId pid,
        uint32 access_flags, ProcessHandle* handle);

    // Closes the process handle opened by OpenProcessHandle.
    void CloseProcessHandle(ProcessHandle process);

    // Returns the unique ID for the specified process. This is functionally the
    // same as Windows' GetProcessId(), but works on versions of Windows before
    // Win XP SP1 as well.
    ProcessId GetProcId(ProcessHandle process);

    // TODO(evan): rename these to use StudlyCaps.
    typedef std::vector<std::pair<std::string, std::string> > environment_vector;
    typedef std::vector<std::pair<int, int> > file_handle_mapping_vector;

    // Options for launching a subprocess that are passed to LaunchProcess().
    // The default constructor constructs the object with default options.
    struct LaunchOptions
    {
        LaunchOptions() : wait(false), start_hidden(false),
            inherit_handles(false), as_user(NULL),
            empty_desktop_name(false), job_handle(NULL) {}

        // If true, wait for the process to complete.
        bool wait;

        bool start_hidden;

        // If true, the new process inherits handles from the parent.
        bool inherit_handles;

        // If non-NULL, runs as if the user represented by the token had launched it.
        // Whether the application is visible on the interactive desktop depends on
        // the token belonging to an interactive logon session.
        //
        // To avoid hard to diagnose problems, when specified this loads the
        // environment variables associated with the user and if this operation fails
        // the entire call fails as well.
        UserTokenHandle as_user;

        // If true, use an empty string for the desktop name.
        bool empty_desktop_name;

        // If non-NULL, launches the application in that job object.
        HANDLE job_handle;
    };

    // Launch a process via the command line |cmdline|.
    // See the documentation of LaunchOptions for details on |options|.
    //
    // If |process_handle| is non-NULL, it will be filled in with the
    // handle of the launched process.  NOTE: In this case, the caller is
    // responsible for closing the handle so that it doesn't leak!
    // Otherwise, the process handle will be implicitly closed.
    //
    // Unix-specific notes:
    // - Before launching, all FDs open in the parent process will be marked as
    //   close-on-exec.
    // - If the first argument on the command line does not contain a slash,
    //   PATH will be searched.  (See man execvp.)
    bool LaunchProcess(const CommandLine& cmdline,
        const LaunchOptions& options,
        ProcessHandle* process_handle);

    enum IntegrityLevel
    {
        INTEGRITY_UNKNOWN,
        LOW_INTEGRITY,
        MEDIUM_INTEGRITY,
        HIGH_INTEGRITY,
    };
    // Determine the integrity level of the specified process. Returns false
    // if the system does not support integrity levels (pre-Vista) or in the case
    // of an underlying system failure.
    bool GetProcessIntegrityLevel(ProcessHandle process, IntegrityLevel* level);

    // Windows-specific LaunchProcess that takes the command line as a
    // string.  Useful for situations where you need to control the
    // command line arguments directly, but prefer the CommandLine version
    // if launching Chrome itself.
    //
    // The first command line argument should be the path to the process,
    // and don't forget to quote it.
    //
    // Example (including literal quotes)
    //  cmdline = "c:\windows\explorer.exe" -foo "c:\bar\"
    bool LaunchProcess(const string16& cmdline,
        const LaunchOptions& options,
        ProcessHandle* process_handle);

    // TODO(evan): deprecated; change callers to use LaunchProcess, remove.
    inline bool LaunchApp(const std::wstring& cmdline,
        bool wait, bool start_hidden,
        ProcessHandle* process_handle)
    {
        LaunchOptions options;
        options.wait = wait;
        options.start_hidden = start_hidden;
        return LaunchProcess(cmdline, options, process_handle);
    }

    // TODO(evan): deprecated; change callers to use LaunchProcess, remove.
    inline bool LaunchAppWithHandleInheritance(const std::wstring& cmdline,
        bool wait, bool start_hidden,
        ProcessHandle* process_handle)
    {
        LaunchOptions options;
        options.wait = wait;
        options.start_hidden = start_hidden;
        options.inherit_handles = true;
        return LaunchProcess(cmdline, options, process_handle);
    }

    // TODO(evan): deprecated; change callers to use LaunchProcess, remove.
    inline bool LaunchAppAsUser(UserTokenHandle token,
        const std::wstring& cmdline,
        bool start_hidden,
        ProcessHandle* process_handle)
    {
        LaunchOptions options;
        options.start_hidden = start_hidden;
        options.as_user = token;
        return LaunchProcess(cmdline, options, process_handle);
    }

    // TODO(evan): deprecated; change callers to use LaunchProcess, remove.
    inline bool LaunchAppAsUser(UserTokenHandle token,
        const std::wstring& cmdline,
        bool start_hidden, ProcessHandle* process_handle,
        bool empty_desktop_name, bool inherit_handles)
    {
        LaunchOptions options;
        options.start_hidden = start_hidden;
        options.as_user = token;
        options.empty_desktop_name = empty_desktop_name;
        options.inherit_handles = inherit_handles;
        return LaunchProcess(cmdline, options, process_handle);
    }

    // TODO(evan): deprecated; change callers to use LaunchProcess, remove.
    // Successfully deprecated on non-Windows.
    inline bool LaunchApp(const CommandLine& cl, bool wait, bool start_hidden,
        ProcessHandle* process_handle)
    {
        LaunchOptions options;
        options.wait = wait;
        options.start_hidden = start_hidden;
        return LaunchProcess(cl, options, process_handle);
    }

    // Executes the application specified by |cl| and wait for it to exit. Stores
    // the output (stdout) in |output|. Redirects stderr to /dev/null. Returns true
    // on success (application launched and exited cleanly, with exit code
    // indicating success).
    bool GetAppOutput(const CommandLine& cl, std::string* output);

    // Used to filter processes by process ID.
    class ProcessFilter
    {
    public:
        // Returns true to indicate set-inclusion and false otherwise.  This method
        // should not have side-effects and should be idempotent.
        virtual bool Includes(const ProcessEntry& entry) const = 0;

    protected:
        virtual ~ProcessFilter() {}
    };

    // Returns the number of processes on the machine that are running from the
    // given executable name.  If filter is non-null, then only processes selected
    // by the filter will be counted.
    int GetProcessCount(const FilePath::StringType& executable_name,
        const ProcessFilter* filter);

    // Attempts to kill all the processes on the current machine that were launched
    // from the given executable name, ending them with the given exit code.  If
    // filter is non-null, then only processes selected by the filter are killed.
    // Returns true if all processes were able to be killed off, false if at least
    // one couldn't be killed.
    bool KillProcesses(const FilePath::StringType& executable_name,
        int exit_code, const ProcessFilter* filter);

    // Attempts to kill the process identified by the given process
    // entry structure, giving it the specified exit code. If |wait| is true, wait
    // for the process to be actually terminated before returning.
    // Returns true if this is successful, false otherwise.
    bool KillProcess(ProcessHandle process, int exit_code, bool wait);

    bool KillProcessById(ProcessId process_id, int exit_code, bool wait);

    // Get the termination status of the process by interpreting the
    // circumstances of the child process' death. |exit_code| is set to
    // the status returned by waitpid() on POSIX, and from
    // GetExitCodeProcess() on Windows.  |exit_code| may be NULL if the
    // caller is not interested in it.  Note that on Linux, this function
    // will only return a useful result the first time it is called after
    // the child exits (because it will reap the child and the information
    // will no longer be available).
    TerminationStatus GetTerminationStatus(ProcessHandle handle, int* exit_code);

    // Waits for process to exit. On POSIX systems, if the process hasn't been
    // signaled then puts the exit code in |exit_code|; otherwise it's considered
    // a failure. On Windows |exit_code| is always filled. Returns true on success,
    // and closes |handle| in any case.
    bool WaitForExitCode(ProcessHandle handle, int* exit_code);

    // Waits for process to exit. If it did exit within |timeout_milliseconds|,
    // then puts the exit code in |exit_code|, closes |handle|, and returns true.
    // In POSIX systems, if the process has been signaled then |exit_code| is set
    // to -1. Returns false on failure (the caller is then responsible for closing
    // |handle|).
    bool WaitForExitCodeWithTimeout(ProcessHandle handle, int* exit_code,
        int64 timeout_milliseconds);

    // Wait for all the processes based on the named executable to exit.  If filter
    // is non-null, then only processes selected by the filter are waited on.
    // Returns after all processes have exited or wait_milliseconds have expired.
    // Returns true if all the processes exited, false otherwise.
    bool WaitForProcessesToExit(const FilePath::StringType& executable_name,
        int64 wait_milliseconds,
        const ProcessFilter* filter);

    // Wait for a single process to exit. Return true if it exited cleanly within
    // the given time limit. On Linux |handle| must be a child process, however
    // on Mac and Windows it can be any process.
    bool WaitForSingleProcess(ProcessHandle handle, int64 wait_milliseconds);

    // Waits a certain amount of time (can be 0) for all the processes with a given
    // executable name to exit, then kills off any of them that are still around.
    // If filter is non-null, then only processes selected by the filter are waited
    // on.  Killed processes are ended with the given exit code.  Returns false if
    // any processes needed to be killed, true if they all exited cleanly within
    // the wait_milliseconds delay.
    bool CleanupProcesses(const FilePath::StringType& executable_name,
        int64 wait_milliseconds, int exit_code,
        const ProcessFilter* filter);

    // This class provides a way to iterate through a list of processes on the
    // current machine with a specified filter.
    // To use, create an instance and then call NextProcessEntry() until it returns
    // false.
    class ProcessIterator
    {
    public:
        typedef std::list<ProcessEntry> ProcessEntries;

        explicit ProcessIterator(const ProcessFilter* filter);
        virtual ~ProcessIterator();

        // If there's another process that matches the given executable name,
        // returns a const pointer to the corresponding PROCESSENTRY32.
        // If there are no more matching processes, returns NULL.
        // The returned pointer will remain valid until NextProcessEntry()
        // is called again or this NamedProcessIterator goes out of scope.
        const ProcessEntry* NextProcessEntry();

        // Takes a snapshot of all the ProcessEntry found.
        ProcessEntries Snapshot();

    protected:
        virtual bool IncludeEntry();
        const ProcessEntry& entry() { return entry_; }

    private:
        // Determines whether there's another process (regardless of executable)
        // left in the list of all processes.  Returns true and sets entry_ to
        // that process's info if there is one, false otherwise.
        bool CheckForNextProcess();

        // Initializes a PROCESSENTRY32 data structure so that it's ready for
        // use with Process32First/Process32Next.
        void InitProcessEntry(ProcessEntry* entry);

        HANDLE snapshot_;
        bool started_iteration_;
        ProcessEntry entry_;
        const ProcessFilter* filter_;

        DISALLOW_COPY_AND_ASSIGN(ProcessIterator);
    };

    // This class provides a way to iterate through the list of processes
    // on the current machine that were started from the given executable
    // name.  To use, create an instance and then call NextProcessEntry()
    // until it returns false.
    class NamedProcessIterator : public ProcessIterator
    {
    public:
        NamedProcessIterator(const FilePath::StringType& executable_name,
            const ProcessFilter* filter);
        virtual ~NamedProcessIterator();

    protected:
        virtual bool IncludeEntry();

    private:
        FilePath::StringType executable_name_;

        DISALLOW_COPY_AND_ASSIGN(NamedProcessIterator);
    };

    // Working Set (resident) memory usage broken down by
    //
    // On Windows:
    // priv (private): These pages (kbytes) cannot be shared with any other process.
    // shareable:      These pages (kbytes) can be shared with other processes under
    //                 the right circumstances.
    // shared :        These pages (kbytes) are currently shared with at least one
    //                 other process.
    //
    // On Linux:
    // priv:           Pages mapped only by this process
    // shared:         PSS or 0 if the kernel doesn't support this
    // shareable:      0
    //
    // On OS X: TODO(thakis): Revise.
    // priv:           Memory.
    // shared:         0
    // shareable:      0
    struct WorkingSetKBytes
    {
        WorkingSetKBytes() : priv(0), shareable(0), shared(0) {}
        size_t priv;
        size_t shareable;
        size_t shared;
    };

    // Committed (resident + paged) memory usage broken down by
    // private: These pages cannot be shared with any other process.
    // mapped:  These pages are mapped into the view of a section (backed by
    //          pagefile.sys)
    // image:   These pages are mapped into the view of an image section (backed by
    //          file system)
    struct CommittedKBytes
    {
        CommittedKBytes() : priv(0), mapped(0), image(0) {}
        size_t priv;
        size_t mapped;
        size_t image;
    };

    // Free memory (Megabytes marked as free) in the 2G process address space.
    // total : total amount in megabytes marked as free. Maximum value is 2048.
    // largest : size of the largest contiguous amount of memory found. It is
    //   always smaller or equal to FreeMBytes::total.
    // largest_ptr: starting address of the largest memory block.
    struct FreeMBytes
    {
        size_t total;
        size_t largest;
        void* largest_ptr;
    };

    // Provides performance metrics for a specified process (CPU usage, memory and
    // IO counters). To use it, invoke CreateProcessMetrics() to get an instance
    // for a specific process, then access the information with the different get
    // methods.
    class ProcessMetrics
    {
    public:
        ~ProcessMetrics();

        // Creates a ProcessMetrics for the specified process.
        // The caller owns the returned object.
        static ProcessMetrics* CreateProcessMetrics(ProcessHandle process);

        // Returns the current space allocated for the pagefile, in bytes (these pages
        // may or may not be in memory).  On Linux, this returns the total virtual
        // memory size.
        size_t GetPagefileUsage() const;
        // Returns the peak space allocated for the pagefile, in bytes.
        size_t GetPeakPagefileUsage() const;
        // Returns the current working set size, in bytes.  On Linux, this returns
        // the resident set size.
        size_t GetWorkingSetSize() const;
        // Returns the peak working set size, in bytes.
        size_t GetPeakWorkingSetSize() const;
        // Returns private and sharedusage, in bytes. Private bytes is the amount of
        // memory currently allocated to a process that cannot be shared. Returns
        // false on platform specific error conditions.  Note: |private_bytes|
        // returns 0 on unsupported OSes: prior to XP SP2.
        bool GetMemoryBytes(size_t* private_bytes, size_t* shared_bytes);
        // Fills a CommittedKBytes with both resident and paged
        // memory usage as per definition of CommittedBytes.
        void GetCommittedKBytes(CommittedKBytes* usage) const;
        // Fills a WorkingSetKBytes containing resident private and shared memory
        // usage in bytes, as per definition of WorkingSetBytes.
        bool GetWorkingSetKBytes(WorkingSetKBytes* ws_usage) const;

        // Computes the current process available memory for allocation.
        // It does a linear scan of the address space querying each memory region
        // for its free (unallocated) status. It is useful for estimating the memory
        // load and fragmentation.
        bool CalculateFreeMemory(FreeMBytes* free) const;

        // Returns the CPU usage in percent since the last time this method was
        // called. The first time this method is called it returns 0 and will return
        // the actual CPU info on subsequent calls.
        // On Windows, the CPU usage value is for all CPUs. So if you have 2 CPUs and
        // your process is using all the cycles of 1 CPU and not the other CPU, this
        // method returns 50.
        double GetCPUUsage();

        // Retrieves accounting information for all I/O operations performed by the
        // process.
        // If IO information is retrieved successfully, the function returns true
        // and fills in the IO_COUNTERS passed in. The function returns false
        // otherwise.
        bool GetIOCounters(IoCounters* io_counters) const;

    private:
        explicit ProcessMetrics(ProcessHandle process);

        ProcessHandle process_;

        int processor_count_;

        // Used to store the previous times and CPU usage counts so we can
        // compute the CPU usage between calls.
        int64 last_time_;
        int64 last_system_time_;

        DISALLOW_COPY_AND_ASSIGN(ProcessMetrics);
    };

    // Returns the memory commited by the system in KBytes.
    // Returns 0 if it can't compute the commit charge.
    size_t GetSystemCommitCharge();

    // Enables low fragmentation heap (LFH) for every heaps of this process. This
    // won't have any effect on heaps created after this function call. It will not
    // modify data allocated in the heaps before calling this function. So it is
    // better to call this function early in initialization and again before
    // entering the main loop.
    // Note: Returns true on Windows 2000 without doing anything.
    bool EnableLowFragmentationHeap();

    // Enables 'terminate on heap corruption' flag. Helps protect against heap
    // overflow. Has no effect if the OS doesn't provide the necessary facility.
    void EnableTerminationOnHeapCorruption();

    // Turns on process termination if memory runs out.
    void EnableTerminationOnOutOfMemory();

    // Enables stack dump to console output on exception and signals.
    // When enabled, the process will quit immediately. This is meant to be used in
    // unit_tests only!
    bool EnableInProcessStackDumping();

    // If supported on the platform, and the user has sufficent rights, increase
    // the current process's scheduling priority to a high priority.
    void RaiseProcessToHighPriority();

} //namespace base

#endif //__base_process_util_h__