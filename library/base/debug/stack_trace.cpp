
#include "stack_trace.h"

#include <windows.h>
#include <dbghelp.h>

#include <iostream>
#include <algorithm>

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"

#pragma comment(lib, "dbghelp.lib")

namespace
{

    // SymbolContext是一个封装了DbgHelp的Sym*系列函数的线程安全的单件.
    // Sym*系列函数同一时间只能被一个线程调用. SymbolContext代码可能会通过网络
    // 访问symbol服务器, 这个时候锁会导致延迟从而影响性能.
    //
    // 还有一个已知的问题就是: 当使用Sym*系列函数时, 在另外一个线程中调用
    // breakpad, backtrace的代码会跟breakpad部分有冲突. 参见bug:
    //   http://code.google.com/p/google-breakpad/issues/detail?id=311
    // 这种是极端情况, 当前不予考虑.
    class SymbolContext
    {
    public:
        static SymbolContext* GetInstance()
        {
            // 采用leaky单件因为代码可能在进程结束时调用.
            return Singleton<SymbolContext,
                LeakySingletonTraits<SymbolContext> >::get();
        }

        // 返回初始化失败的错误码.
        DWORD init_error() const
        {
            return init_error_;
        }

        // 对于指定的trace, 尝试解析符号并输出到os流中. 回溯的每行格式如下:
        //     <tab>SymbolName[0xAddress+Offset] (FileName:LineNo)
        // 必须在Init()之后调用. 在这里不要调用LOG(FATAL), 因为可能就是LOG(FATAL)
        // 调用过来的, 这样会引起死循环.
        void OutputTraceToStream(const void* const* trace, int count,
            std::ostream* os)
        {
            base::AutoLock lock(lock_);

            for(size_t i=0; (i<static_cast<size_t>(count))&&os->good(); ++i)
            {
                const int kMaxNameLength = 256;
                DWORD_PTR frame = reinterpret_cast<DWORD_PTR>(trace[i]);

                // 代码是MSDN例子改写的:
                // http://msdn.microsoft.com/en-us/library/ms680578(VS.85).aspx
                ULONG64 buffer[(sizeof(SYMBOL_INFO)+
                    kMaxNameLength*sizeof(wchar_t)+sizeof(ULONG64)-1)/sizeof(ULONG64)];
                memset(buffer, 0, sizeof(buffer));

                // 初始化符号信息结构体.
                DWORD64 sym_displacement = 0;
                PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(&buffer[0]);
                symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbol->MaxNameLen = kMaxNameLength - 1;
                BOOL has_symbol = SymFromAddr(GetCurrentProcess(), frame,
                    &sym_displacement, symbol);

                // 尝试返回行信息.
                DWORD line_displacement = 0;
                IMAGEHLP_LINE64 line = { 0 };
                line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                BOOL has_line = SymGetLineFromAddr64(GetCurrentProcess(), frame,
                    &line_displacement, &line);

                // 输出回溯行信息.
                (*os) << "\t";
                if(has_symbol)
                {
                    (*os) << symbol->Name << " [0x" << trace[i] << "+"
                        << sym_displacement << "]";
                }
                else
                {
                    // 没有符号信息.
                    (*os) << "(No symbol) [0x" << trace[i] << "]";
                }
                if(has_line)
                {
                    (*os) << " (" << line.FileName << ":" << line.LineNumber << ")";
                }
                (*os) << "\n";
            }
        }

    private:
        friend struct DefaultSingletonTraits<SymbolContext>;

        SymbolContext() : init_error_(ERROR_SUCCESS)
        {
            // 初始化进程的符号表.
            // 需要时延迟加载符号, 所有符号使用未修饰方式, 加载行号信息.
            SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_UNDNAME|SYMOPT_LOAD_LINES);
            if(SymInitialize(GetCurrentProcess(), NULL, TRUE))
            {
                init_error_ = ERROR_SUCCESS;
            }
            else
            {
                init_error_ = GetLastError();
                DLOG(ERROR) << "SymInitialize failed: " << init_error_;
            }
        }

        DWORD init_error_;
        base::Lock lock_;
        DISALLOW_COPY_AND_ASSIGN(SymbolContext);
    };

}

namespace base
{
    namespace debug
    {

        // Disable optimizations for the StackTrace::StackTrace function. It is
        // important to disable at least frame pointer optimization ("y"), since
        // that breaks CaptureStackBackTrace() and prevents StackTrace from working
        // in Release builds (it may still be janky if other frames are using FPO,
        // but at least it will make it further).
        #pragma optimize("", off)

        StackTrace::StackTrace()
        {
            // 使用CaptureStackBackTrace()获得调用堆栈信息.
            count_ = CaptureStackBackTrace(0, arraysize(trace_), trace_, NULL);
        }

        #pragma optimize("", on)

        StackTrace::StackTrace(_EXCEPTION_POINTERS* exception_pointers)
        {
            // 使用StackWalk64()获得异常堆栈信息.
            count_ = 0;
            // 初始化堆栈查核行程.
            STACKFRAME64 stack_frame;
            memset(&stack_frame, 0, sizeof(stack_frame));
#if defined(_WIN64)
            int machine_type = IMAGE_FILE_MACHINE_AMD64;
            stack_frame.AddrPC.Offset = exception_pointers->ContextRecord->Rip;
            stack_frame.AddrFrame.Offset = exception_pointers->ContextRecord->Rbp;
            stack_frame.AddrStack.Offset = exception_pointers->ContextRecord->Rsp;
#else
            int machine_type = IMAGE_FILE_MACHINE_I386;
            stack_frame.AddrPC.Offset = exception_pointers->ContextRecord->Eip;
            stack_frame.AddrFrame.Offset = exception_pointers->ContextRecord->Ebp;
            stack_frame.AddrStack.Offset = exception_pointers->ContextRecord->Esp;
#endif
            stack_frame.AddrPC.Mode = AddrModeFlat;
            stack_frame.AddrFrame.Mode = AddrModeFlat;
            stack_frame.AddrStack.Mode = AddrModeFlat;
            while(StackWalk64(machine_type,
                GetCurrentProcess(),
                GetCurrentThread(),
                &stack_frame,
                exception_pointers->ContextRecord,
                NULL,
                &SymFunctionTableAccess64,
                &SymGetModuleBase64,
                NULL) && count_<arraysize(trace_))
            {
                trace_[count_++] = reinterpret_cast<void*>(stack_frame.AddrPC.Offset);
            }
        }

        StackTrace::StackTrace(const void* const* trace, size_t count)
        {
            count = std::min(count, arraysize(trace_));
            if(count)
            {
                memcpy(trace_, trace, count*sizeof(trace_[0]));
            }
            count_ = static_cast<int>(count);
        }

        StackTrace::~StackTrace() {}

        const void* const* StackTrace::Addresses(size_t* count)
        {
            *count = count_;
            if(count_)
            {
                return trace_;
            }
            return NULL;
        }

        void StackTrace::PrintBacktrace()
        {
            OutputToStream(&std::cerr);
        }

        void StackTrace::OutputToStream(std::ostream* os)
        {
            SymbolContext* context = SymbolContext::GetInstance();
            DWORD error = context->init_error();
            if(error != ERROR_SUCCESS)
            {
                (*os) << "Error initializing symbols (" << error
                    << ").  Dumping unresolved backtrace:\n";
                for(int i=0; (i<count_)&&os->good(); ++i)
                {
                    (*os) << "\t" << trace_[i] << "\n";
                }
            }
            else
            {
                (*os) << "Backtrace:\n";
                context->OutputTraceToStream(trace_, count_, os);
            }
        }

    } //namespace debug
} //namespace base