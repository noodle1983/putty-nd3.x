
#ifndef __base_stack_trace_h__
#define __base_stack_trace_h__

// 调试器相关的辅助函数. 用于测试程序是否在调试器下运行以及
// 是否需要在调试器里中断.

#pragma once

#include <iosfwd>

struct _EXCEPTION_POINTERS;

namespace base
{
    namespace debug
    {

        // 堆栈跟踪在调试的时候非常有用. 比如在对象中添加StackTrace成员(
        // 一般在#ifndef NDEBUG里), 这样就能知道对象是在哪里创建的.
        class StackTrace
        {
        public:
            // 在当前位置构造一个stacktrace.
            StackTrace();

            // 缺省的拷贝构造和赋值构造可以正常工作.

            // 为异常创建stacktrace.
            // 注意: 这个函数在没有dbghelp 5.1的系统上会抛出找不到入口点
            // (an import not found (StackWalk64))的异常.
            StackTrace(_EXCEPTION_POINTERS* exception_pointers);

            // Creates a stacktrace from an existing array of instruction
            // pointers (such as returned by Addresses()).  |count| will be
            // trimmed to |kMaxTraces|.
            StackTrace(const void* const* trace, size_t count);

            // Copying and assignment are allowed with the default functions.
            ~StackTrace();

            // 获得堆栈信息数组
            //   count: 返回堆栈信息的数量.
            const void* const* Addresses(size_t* count);
            // 打印堆栈回溯信息到标准输出.
            void PrintBacktrace();
            // 回溯解析符号表并写入流中.
            void OutputToStream(std::ostream* os);

        private:
            // 参见http://msdn.microsoft.com/en-us/library/bb204633.aspx,
            // FramesToSkip和FramesToCapture之和必须小于63, 所以设置堆栈跟踪
            // 的最大回溯值为62, 即使其它平台能提供更大的值, 一般也没什么意义.
            static const int MAX_TRACES = 62;
            void* trace_[MAX_TRACES];
            int count_;
        };

    } //namespace debug
} //namespace base

#endif //__base_stack_trace_h__