
#ifndef __base_logging_h__
#define __base_logging_h__

#pragma once

#include <string>
#include <sstream>

#include "basic_types.h"

// 可选的弹出消息方案
// ------------------
// 断言失败消息(assertion failed messages)和致命错误(fatal errors)在程序退出之前
// 显示在对话框中. 运行对话框创建的消息循环会处理应用程序消息并分发到相应的窗口.
// 断言对话框出现时应用本身已经处于非常糟糕的状态, 这些消息可能得不到处理而堵塞
// 对话框或者导致程序出现更多的错误.
//
// 因此主程序通过一个单独的进程来显示错误对话框是一种好的解决办法, 当logging system
// 需要显示致命错误对话框的时候在运行目录下查找可执行程序"DebugMessage.exe". 通过
// 命令行的方式创建"DebugMessage.exe"进程, 为了解析方便通常参数都不包含主程序名.
//
// DebugMessage.exe的WinMain函数里面基本上只有一行代码:
//     MessageBoxW(NULL, GetCommandLineW(), L"Fatal Error", 0);
//
// 如果没有找到可执行程序"DebugMessage.exe", 就采用上面讨论过的会导致问题的方式,
// 直接弹出MessageBox.


// 使用说明
// --------
// 通过一系列宏来实现日志(logging). 打日志的方式是流到LOG(<a particular severity level>)
// 例如:
//     LOG(INFO) << "Found " << num_cookies << " cookies";
//
// 也可以加上条件限制:
//     LOG_IF(INFO, num_cookies>10) << "Got lots of cookies";
//
// CHECK(condition)宏在debug和release版本中都起作用, 通过LOG(FATAL)结束进程并且会
// 生成crashdump, 除非进程已经附加了调试器.
//
// 有一些宏是作用在debug模式下的, 像下面这两个就是:
//     DLOG(INFO) << "Found cookies";
//     DLOG_IF(INFO, num_cookies>10) << "Got lots of cookies";
//
// 所有debug模式下有效的宏在非debug版本编译中会被忽略, 不会生成任何指令. LOG_IF和
// 条件开关也能够正常工作因为代码会被编译器忽略.
//
// 有一些常用的语法糖宏:
//     LOG_ASSERT(assertion);
//     DLOG_ASSERT(assertion);
// 等同于:
//     {,D}LOG_IF(FATAL, assert fails) << assertion;
//
// 还有一些"详细等级"的日志宏, 如下
//     VLOG(1) << "I'm printed when you run the program with --v=1 or more";
//     VLOG(2) << "I'm printed when you run the program with --v=2 or more";
// 日志输出通常是INFO等级(全部不输出).
// 详细日志可以按照模块开启, 例如
//     --vmodule=profile=2,icon_loader=1,browser_*=3 --v=0
// 会导致:
//     a. 在profile.{h,cpp}中VLOG(2)和底等级的消息会输出
//     b. 在icon_loader.{h,cpp}中VLOG(1)和底等级的消息会输出
//     c. 以"browser"为前缀的文件中VLOG(3)和底等级的消息会输出
//     d. 任意地方的VLOG(0)和底等级的消息会输出
//
// (c)中展示了'*'(适配0或者多个字符)和'?'(适配1个字符)通配符功能. 含有\或者/的
// 模式会匹配整个路径而不是一个模块. 例如"*/foo/bar/*=2"会改变"foo/bar"目录下
// 所有源文件中代码的日志等级.
//
// 有"详细等级"条件宏VLOG_IS_ON(n), 用于
//     if(VLOG_IS_ON(2)) {
//       // 做一些VLOG(2) << ...;单独无法实现的日志记录
//     }
// 下面是"详细等级"条件宏VLOG_IF的示例, 根据条件求值判断是否需要进行日志:
//     VLOG_IF(1, (size>1024))
//       << "I'm printed when size is more than 1024 and when you run the "
//         "program with --v=1 or more";
//
// 替代标准的'assert'为'DLOG_ASSERT'.
//
// 最后, 有一类宏会附带上最后一次系统调用错误(GetLastError())描述:
//     PLOG(ERROR) << "Couldn't do foo";
//     DPLOG(ERROR) << "Couldn't do foo";
//     PLOG_IF(ERROR, cond) << "Couldn't do foo";
//     DPLOG_IF(ERROR, cond) << "Couldn't do foo";
//     PCHECK(condition) << "Couldn't do foo";
//     DPCHECK(condition) << "Couldn't do foo";
//
// 宏支持的严重等级有(严重等级递增序)INFO, WARNING, ERROR, ERROR_REPORT和FATAL.
//
// 需要提醒的是: 记录一条严重等级为FATAL的消息会导致程序终止(消息记录之后).
//
// 注意release模式下严重等级为ERROR_REPORT会显示错误对话框但不会终止程序, 
// 严重等级为ERROR以及以下的不会显示错误对话框.
//
// DFATAL严重等级比较特别, 在debug模式下等同FATAL, release模式下等同ERROR.

namespace base
{

    // 日志输出到哪里? 文件和/或通过OutputDebugString打到系统调试输出.
    // 缺省是LOG_ONLY_TO_FILE.
    enum LoggingDestination
    {
        LOG_NONE,
        LOG_ONLY_TO_FILE,
        LOG_ONLY_TO_SYSTEM_DEBUG_LOG,
        LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG
    };

    // 指定写日志文件时是否需要加锁.
    // 单线程程序写日志不需要加锁, 对于多线程程序而言可能同时有多个线程
    // 写日志, 写操作时需要对文件加锁以保证每条记录都是原子的. 
    // 所有写日志文件的进程都必须遵守加锁机制以便正常记录日志.
    // 缺省是DONT_LOCK_LOG_FILE.
    enum LogLockingState { LOCK_LOG_FILE, DONT_LOCK_LOG_FILE };

    // 启动时, 删除老日志文件还是附加在老日志文件上?
    // 缺省是APPEND_TO_OLD_LOG_FILE
    enum OldFileDeletionState { DELETE_OLD_LOG_FILE, APPEND_TO_OLD_LOG_FILE };

    enum DcheckState
    {
        DISABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS,
        ENABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS
    };

    typedef wchar_t PathChar;

    // 根据是否定义NDEBUG宏定义不同的BaseInitLoggingImpl()函数名, 防止编译库和使用库
    // 时候NDEBUG定义的不一致.
#if NDEBUG
#define BaseInitLoggingImpl BaseInitLoggingImpl_built_with_NDEBUG
#else
#define BaseInitLoggingImpl BaseInitLoggingImpl_built_without_NDEBUG
#endif

    // 设置日志文件名和其它全局状态, 建议在程序初始化的时候调用此函数.
    // 如果不调用, 所有状态标记都是缺省值, 且第一次写日志假如有2个线程同时进行,
    // 初始化critical section对象时会发生冲突并导致泄漏.
    // 缺省值参见枚举定义.
    // 缺省日志文件名是"debug.log", 位于程序所在目录下. 一般情况下不希望如此,
    // 尤其是程序目录在最终用户的系统上不一定可写.
    void BaseInitLoggingImpl(const PathChar* log_file,
        LoggingDestination logging_dest,
        LogLockingState lock_log,
        OldFileDeletionState delete_old,
        DcheckState dcheck_state);

    // 设置日志等级, 高于等于该等级的记录会写文件/显示给用户, 低等级的
    // 记录会被忽略掉, 缺省等级是0(达到INFO等级的日志都会被记录).
    // 注意VLOG(x)日志实际记录的是-x等级, 所以启动详细日志需要设置负值.
    void SetMinLogLevel(int level);

    // 获取当前日志等级.
    int GetMinLogLevel();

    // 获取VLOG的缺省冗余等级.
    int GetVlogVerbosity();

    // 获取指定文件(一般是__FILE__)的当前vlog等级.

    // 注意|N|是包含结尾null的大小.
    int GetVlogLevelHelper(const char* file_start, size_t N);

    template<size_t N>
    int GetVlogLevel(const char (&file)[N])
    {
        return GetVlogLevelHelper(file, N);
    }

    // 设置每条消息的前缀显示内容. 进程和线程ID默认关闭(不显示), 时间戳默认打开.
    // 缺省情况下, 日志记录中只包括时间戳前缀部分.
    void SetLogItems(bool enable_process_id, bool enable_thread_id,
        bool enable_timestamp, bool enable_tickcount);

    // 设置是否需要弹出错误消息对话框. 缺省不弹出.
    void SetShowErrorDialogs(bool enable_dialogs);

    // 设置FATAL处理函数用来接收失败通知. 缺省的处理过程是显示错误消息框并
    // 结束进程, 客户端可以设置自己的处理进行重载(比如单元测试时不中断程序).
    typedef void (*LogAssertHandlerFunction)(const std::string& str);
    void SetLogAssertHandler(LogAssertHandlerFunction handler);

    // 设置ERROR报告函数用来接收非debug模式下失败通知. 缺省的处理过程是显示
    // 错误消息框后继续执行. 客户端可以重载自己的处理实现.
    typedef void (*LogReportHandlerFunction)(const std::string& str);
    void SetLogReportHandler(LogReportHandlerFunction handler);

    // 设置日志消息处理函数用来预处理日志消息. 返回true表明已处理不需要再传递到
    // 其他任何日志消息处理函数.
    typedef bool (*LogMessageHandlerFunction)(int severity, const char* file,
        int line, size_t message_start, const std::string& str);
    void SetLogMessageHandler(LogMessageHandlerFunction handler);
    LogMessageHandlerFunction GetLogMessageHandler();

    typedef int LogSeverity;
    const LogSeverity LOG_VERBOSE = -1; // 冗余等级1.
    // 注意: 日志严重等级通过名称索引.
    const LogSeverity LOG_INFO = 0;
    const LogSeverity LOG_WARNING = 1;
    const LogSeverity LOG_ERROR = 2;
    const LogSeverity LOG_ERROR_REPORT = 3;
    const LogSeverity LOG_FATAL = 4;
    const LogSeverity LOG_NUM_SEVERITIES = 5;
    // LOG_DFATAL在debug模式下等同于LOG_FATAL, release模式下等同于ERROR.
#ifdef NDEBUG
    const LogSeverity LOG_DFATAL = LOG_ERROR;
#else
    const LogSeverity LOG_DFATAL = LOG_FATAL;
#endif

    // 一些LOG和LOG_IF用到的代码的宏封装, 由于到处都会用到, 这样做可以保证代码整洁.
#define COMPACT_LOG_EX_INFO(ClassName, ...) \
    base::ClassName(__FILE__, __LINE__, base::LOG_INFO, ##__VA_ARGS__)
#define COMPACT_LOG_EX_WARNING(ClassName, ...) \
    base::ClassName(__FILE__, __LINE__, base::LOG_WARNING, ##__VA_ARGS__)
#define COMPACT_LOG_EX_ERROR(ClassName, ...) \
    base::ClassName(__FILE__, __LINE__, base::LOG_ERROR, ##__VA_ARGS__)
#define COMPACT_LOG_EX_ERROR_REPORT(ClassName, ...) \
    base::ClassName(__FILE__, __LINE__, base::LOG_ERROR_REPORT, ##__VA_ARGS__)
#define COMPACT_LOG_EX_FATAL(ClassName, ...) \
    base::ClassName(__FILE__, __LINE__, base::LOG_FATAL, ##__VA_ARGS__)
#define COMPACT_LOG_EX_DFATAL(ClassName, ...) \
    base::ClassName(__FILE__, __LINE__, base::LOG_DFATAL, ##__VA_ARGS__)

#define COMPACT_LOG_INFO                    COMPACT_LOG_EX_INFO(LogMessage)
#define COMPACT_LOG_WARNING                 COMPACT_LOG_EX_WARNING(LogMessage)
#define COMPACT_LOG_ERROR                   COMPACT_LOG_EX_ERROR(LogMessage)
#define COMPACT_LOG_ERROR_REPORT            COMPACT_LOG_EX_ERROR_REPORT(LogMessage)
#define COMPACT_LOG_FATAL                   COMPACT_LOG_EX_FATAL(LogMessage)
#define COMPACT_LOG_DFATAL                  COMPACT_LOG_EX_DFATAL(LogMessage)

    // wingdi.h中将ERROR定义为0. LOG(ERROR)调用会被展开成COMPACT_LOG_0.
    // 为了保证语法的一致性, 我们需要定义此宏等同于COMPACT_LOG_ERROR.
#define ERROR 0
#define COMPACT_LOG_EX_0(ClassName, ...)    COMPACT_LOG_EX_ERROR(ClassName, ##__VA_ARGS__)
#define COMPACT_LOG_0                       COMPACT_LOG_ERROR
    // LOG_IS_ON(ERROR)需要.
    const LogSeverity LOG_0 = LOG_ERROR;

    // LOG_IS_ON(ERROR_REPORT)和LOG_IS_ON(FATAL)总是成立的, LOG_IS_ON(DFATAL)
    // 在debug模式下总是成立的. CHECK()s在条件不成立时总会触发.
#define LOG_IS_ON(severity)                 ((base::LOG_ ## severity) >= base::GetMinLogLevel())

    // 与--vmodule相关的v-logging函数可能会比较慢.
#define VLOG_IS_ON(verboselevel)            ((verboselevel) <= base::GetVlogLevel(__FILE__))

    // 辅助宏, 在condition不成立的情况下把stream参数转换为空.
#define LAZY_STREAM(stream, condition) \
    !(condition) ? (void)0 : base::LogMessageVoidify()&(stream)

    // 使用预处理器的连接操作符号"##", 这样LOG(INFO)就被展开成COMPACT_LOG_INFO.
    // ostream的流操作成员函数(例如: ostream::operator<<(int))和ostream的流操作
    // 非成员函数(例如: ::operator<<(ostream&, string&))之间有些许有趣的差别:
    // 无法将类似字符串这样的内容直接流入未命名的ostream. 通过调用LogMessage的
    // stream()成员函数看起来可以巧妙的避开这个问题.
#define LOG_STREAM(severity)                COMPACT_LOG_ ## severity.stream()
#define LOG(severity)                       LAZY_STREAM(LOG_STREAM(severity), LOG_IS_ON(severity))
#define LOG_IF(severity, condition)         LAZY_STREAM(LOG_STREAM(severity), LOG_IS_ON(severity)&&(condition))

#define VLOG_STREAM(verbose_level)          base::LogMessage(__FILE__, __LINE__, -verbose_level).stream()
#define VLOG(verbose_level)                 LAZY_STREAM(VLOG_STREAM(verbose_level), VLOG_IS_ON(verbose_level))
#define VLOG_IF(verbose_level, condition)   LAZY_STREAM(VLOG_STREAM(verbose_level), VLOG_IS_ON(verbose_level)&&(condition))

#define LOG_ASSERT(condition)               LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "

#define LOG_GETLASTERROR_STREAM(severity)   COMPACT_LOG_EX_ ## severity(Win32ErrorLogMessage, base::GetLastSystemErrorCode()).stream()
#define LOG_GETLASTERROR(severity)          LAZY_STREAM(LOG_GETLASTERROR_STREAM(severity), LOG_IS_ON(severity))
#define LOG_GETLASTERROR_MODULE_STREAM(severity, module) \
    COMPACT_LOG_EX_ ## severity(Win32ErrorLogMessage, base::GetLastSystemErrorCode(), module).stream()
#define LOG_GETLASTERROR_MODULE(severity, module) \
    LAZY_STREAM(LOG_GETLASTERROR_MODULE_STREAM(severity, module), LOG_IS_ON(severity))
    
    // PLOG使用PLOG_STREAM, 它是各平台错误日志最常用的宏.
#define PLOG_STREAM(severity)               LOG_GETLASTERROR_STREAM(severity)
#define PLOG(severity)                      LAZY_STREAM(PLOG_STREAM(severity), LOG_IS_ON(severity))
#define PLOG_IF(severity, condition)        LAZY_STREAM(PLOG_STREAM(severity), LOG_IS_ON(severity)&&(condition))

    // CHECK在条件不成立的情况下会引起致命错误, 它不受NDEBUG控制, 在任何编译条件
    // 下都会执行.
    //
    // 确保CHECK宏对参数求值, 例如常见用法CHECK(FunctionWithSideEffect()).
#define CHECK(condition)                    LAZY_STREAM(LOG_STREAM(FATAL), !(condition)) << "Check failed: " #condition ". "
#define PCHECK(condition)                   LAZY_STREAM(PLOG_STREAM(FATAL), !(condition)) << "Check failed: " #condition ". "

    // 错误消息串构建. 
    template<typename t1, typename t2>
    std::string* MakeCheckOpString(const t1& v1, const t2& v2, const char* names)
    {
        std::ostringstream ss;
        ss << names << " (" << v1 << " vs. " << v2 << ")";
        std::string* msg = new std::string(ss.str());
        return msg;
    }

    // 二进制操作辅助宏. 不要在代码中直接使用, 请用CHECK_EQ替代.
#define CHECK_OP(name, op, val1, val2) \
    if(std::string* _result = \
        base::Check##name##Impl((val1), (val2), #val1 " " #op " " #val2)) \
        base::LogMessage(__FILE__, __LINE__, _result).stream()

    // CHECK_OP宏实现辅助函数. (int, int)特例化版本是为了解决编译器无法
    // 实例化未命名enum类型版本函数.
#define DEFINE_CHECK_OP_IMPL(name, op) \
    template<typename t1, typename t2> \
    inline std::string* Check##name##Impl(const t1& v1, const t2& v2, const char* names) \
    { \
        if(v1 op v2) return NULL; \
        else return MakeCheckOpString(v1, v2, names); \
    } \
    inline std::string* Check##name##Impl(int v1, int v2, const char* names) \
    { \
        if(v1 op v2) return NULL; \
        else return MakeCheckOpString(v1, v2, names); \
    }
    
DEFINE_CHECK_OP_IMPL(EQ, ==)
DEFINE_CHECK_OP_IMPL(NE, !=)
DEFINE_CHECK_OP_IMPL(LE, <=)
DEFINE_CHECK_OP_IMPL(LT, < )
DEFINE_CHECK_OP_IMPL(GE, >=)
DEFINE_CHECK_OP_IMPL(GT, > )
#undef DEFINE_CHECK_OP_IMPL

#define CHECK_EQ(val1, val2)    CHECK_OP(EQ, ==, val1, val2)
#define CHECK_NE(val1, val2)    CHECK_OP(NE, !=, val1, val2)
#define CHECK_LE(val1, val2)    CHECK_OP(LE, <=, val1, val2)
#define CHECK_LT(val1, val2)    CHECK_OP(LT, < , val1, val2)
#define CHECK_GE(val1, val2)    CHECK_OP(GE, >=, val1, val2)
#define CHECK_GT(val1, val2)    CHECK_OP(GT, > , val1, val2)

#if (defined(OS_WIN) && defined(OFFICIAL_BUILD))
#define LOGGING_IS_OFFICIAL_BUILD

    // 为了优化代码, 官方版本编译时移除DLOGs和DCHECKs.
#define ENABLE_DLOG     0
#define ENABLE_DCHECK   0
#elif defined(NDEBUG)
    // 在release编译时移除DLOGs保留DCHECKs(仍可通过命令行标志开启).
#define ENABLE_DLOG     0
#define ENABLE_DCHECK   1
#else
    // debug编译时启用DLOGs和DCHECKs.
#define ENABLE_DLOG     1
#define ENABLE_DCHECK   1
#endif

    // 定义DLOG相关.
#if ENABLE_DLOG
#define DLOG_IS_ON(severity)                LOG_IS_ON(severity)
#define DLOG_IF(severity, condition)        LOG_IF(severity, condition)
#define DLOG_ASSERT(condition)              LOG_ASSERT(condition)
#define DPLOG_IF(severity, condition)       PLOG_IF(severity, condition)
#define DVLOG_IF(verboselevel, condition)   VLOG_IF(verboselevel, condition)
#else //ENABLE_DLOG
    // 如果ENABLE_DLOG关闭, 需要忽略所有的|condition|引用(变量可能只在
    // 非NDEBUG下定义), 这点跟DCHECK相关宏有差别.
#define DLOG_EAT_STREAM_PARAMETERS          true ? (void)0 : base::LogMessageVoidify()&LOG_STREAM(FATAL)

#define DLOG_IS_ON(severity)                false
#define DLOG_IF(severity, condition)        DLOG_EAT_STREAM_PARAMETERS
#define DLOG_ASSERT(condition)              DLOG_EAT_STREAM_PARAMETERS
#define DPLOG_IF(severity, condition)       DLOG_EAT_STREAM_PARAMETERS
#define DVLOG_IF(verboselevel, condition)   DLOG_EAT_STREAM_PARAMETERS
#endif //ENABLE_DLOG

    // 用户可以这样使用DEBUG_MODE
    //   if(DEBUG_MODE) foo.CheckThatFoo();
    // 来替换
    //   #ifndef NDEBUG
    //     foo.CheckThatFoo();
    //   #endif
    //
    // DEBUG_MODE与ENABLE_DLOG一致.
    enum { DEBUG_MODE = ENABLE_DLOG };

#undef ENABLE_DLOG

#define DLOG(severity)                      LAZY_STREAM(LOG_STREAM(severity), DLOG_IS_ON(severity))

#define DLOG_GETLASTERROR(severity)         LAZY_STREAM(LOG_GETLASTERROR_STREAM(severity), DLOG_IS_ON(severity))
#define DLOG_GETLASTERROR_MODULE(severity, module) \
    LAZY_STREAM(LOG_GETLASTERROR_MODULE_STREAM(severity, module), DLOG_IS_ON(severity))

#define DPLOG(severity)                     LAZY_STREAM(PLOG_STREAM(severity), DLOG_IS_ON(severity))
#define DVLOG(verboselevel)                 DLOG_IF(INFO, VLOG_IS_ON(verboselevel))

    // 定义DCHECK相关.
#if ENABLE_DCHECK

#if defined(NDEBUG)
#define COMPACT_LOG_EX_DCHECK(ClassName, ...) \
    COMPACT_LOG_EX_ERROR_REPORT(ClassName , ##__VA_ARGS__)
#define COMPACT_LOG_DCHECK COMPACT_LOG_ERROR_REPORT
    const LogSeverity LOG_DCHECK = LOG_ERROR_REPORT;
    extern DcheckState g_dcheck_state;
#define DCHECK_IS_ON() ((base::g_dcheck_state == \
    base::ENABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS) && \
        LOG_IS_ON(DCHECK))
#else //defined(NDEBUG)
    // 一般debug编译时启用DCHECKS.
#define COMPACT_LOG_EX_DCHECK(ClassName, ...) \
    COMPACT_LOG_EX_FATAL(ClassName , ##__VA_ARGS__)
#define COMPACT_LOG_DCHECK COMPACT_LOG_FATAL
    const LogSeverity LOG_DCHECK = LOG_FATAL;
#define DCHECK_IS_ON() true
#endif //defined(NDEBUG)

#else //!ENABLE_DCHECK

#define COMPACT_LOG_EX_DCHECK(ClassName, ...) \
    COMPACT_LOG_EX_INFO(ClassName , ##__VA_ARGS__)
#define COMPACT_LOG_DCHECK COMPACT_LOG_INFO
    const LogSeverity LOG_DCHECK = LOG_INFO;
#define DCHECK_IS_ON() false

#endif //ENABLE_DCHECK
#undef ENABLE_DCHECK

    // 不像CHECK函数, DCHECK函数只是在必要的情况下对参数求值.
    // DCHECK函数不管DCHECKs是否启用都会引用|condition|; 所以不会得到
    // 变量未使用的警告因为至少在DCHECK中使用过, 这点与DLOG_IF不同.

#define DCHECK(condition) \
    LAZY_STREAM(LOG_STREAM(DCHECK), DCHECK_IS_ON() && !(condition)) \
        << "Check failed: " #condition ". "

#define DPCHECK(condition) \
    LAZY_STREAM(PLOG_STREAM(DCHECK), DCHECK_IS_ON() && !(condition)) \
        << "Check failed: " #condition ". "

    // 二进制操作辅助宏. 不要在代码中直接使用, 请用CHECK_EQ替代.
#define DCHECK_OP(name, op, val1, val2) \
    if(DCHECK_IS_ON()) \
    if(std::string* _result = \
        base::Check##name##Impl((val1), (val2), #val1 " " #op " " #val2)) \
        base::LogMessage(__FILE__, __LINE__, base::LOG_DCHECK, _result).stream()

    // 两个值相等/不等比较且在非预期结果时记录一条包含两个值的LOG_FATAL消息日志.
    // 值必须有operator<<(ostream, ...)操作符重载版本.
    //
    // 可以像这样使用:
    //     DCHECK_NE(1, 2) << ": The world must be ending!";
    //
    // 这些宏可以保证每个传入参数都能被正确计算, 所有合法的参数都能够正确处理.
    // 临时参数也可以正常工作, 例如:
    //     DCHECK_EQ(string("abc")[1], 'b');
    //
    // 警告: 指针和NULL这样的参数可能会导致无法编译通过, 可以强制转换(static_cast)
    // NULL到对应的指针类型.
#define DCHECK_EQ(val1, val2)        DCHECK_OP(EQ, ==, val1, val2)
#define DCHECK_NE(val1, val2)        DCHECK_OP(NE, !=, val1, val2)
#define DCHECK_LE(val1, val2)        DCHECK_OP(LE, <=, val1, val2)
#define DCHECK_LT(val1, val2)        DCHECK_OP(LT, < , val1, val2)
#define DCHECK_GE(val1, val2)        DCHECK_OP(GE, >=, val1, val2)
#define DCHECK_GT(val1, val2)        DCHECK_OP(GT, > , val1, val2)

#define NOTREACHED() DCHECK(false)

        // 重定义标准库的assert实现更好的日志.
#undef assert
#define assert(x) DLOG_ASSERT(x)

        // LogMessage是日志消息类. 实例化一个对象然后流入一些信息, 对象析构的时候,
        // 所有的消息都被流入到合适的地方.
        //
        // 没有必要直接构造对象进行消息流操作, LOG()这些宏完成了这类封装.
    class LogMessage
    {
    public:
        LogMessage(const char* file, int line, LogSeverity severity, int ctr);

        // 下面2个构造函数是为了LOG调用的简化版本.
        //
        // 在LOG(INFO)中使用:
        // 默认: severity = LOG_INFO, ctr = 0
        //
        // 使用这个版本替换复杂构造版本可以减少几个字节的调用消耗.
        LogMessage(const char* file, int line);

        // 在LOG(severity)中使用(severity != INFO):
        // 默认: ctr = 0
        //
        // 使用这个版本替换复杂构造版本可以减少几个字节的调用消耗.
        LogMessage(const char* file, int line, LogSeverity severity);

        // 带有失败检验参数的构造版本.
        // 默认: severity = LOG_FATAL
        LogMessage(const char* file, int line, std::string* result);

        // 带有严重等级和失败检验参数的构造版本.
        LogMessage(const char* file, int line, LogSeverity severity,
            std::string* result);

        ~LogMessage();

        std::ostream& stream() { return stream_; }

    private:
        void Init(const char* file, int line);

        LogSeverity severity_;
        std::ostringstream stream_;
        size_t message_start_; // 日志消息的起始偏移(有可能有前缀内容: 进程id 线程id ...).

        // 传递到构造函数的文件和行号信息.
        const char* file_;
        const int line_;

        // SaveLastError类存储当前GetLastError的值并在析构函数中恢复.
        // LogMessage类自身调用了大量的Win32函数, 调用日志函数的代码在函数
        // 返回时GLE值已经被改变.
        class SaveLastError
        {
        public:
            SaveLastError();
            ~SaveLastError();

            unsigned long get_error() const { return last_error_; }

        protected:
            unsigned long last_error_;
        };

        SaveLastError last_error_;

        DISALLOW_COPY_AND_ASSIGN(LogMessage);
    };

    // 日志记录函数接口(当日志记录等级是变量的时候可以用到)
    inline void LogAtLevel(const int log_level, const std::string& msg)
    {
        LogMessage(__FILE__, __LINE__, log_level).stream() << msg;
    }

    // LogMessageVoidify类用来在条件日志宏中显式忽略一些值, 避免编译警告,
    // 诸如: "value computed is not used"和"statement has no effect".
    class LogMessageVoidify
    {
    public:
        LogMessageVoidify() {}
        // &优先级低于<<但是高于?:
        void operator&(std::ostream&) {}
    };

    typedef unsigned long SystemErrorCode;
    SystemErrorCode GetLastSystemErrorCode();

    // 附加GetLastError()错误格式化后字符串
    class Win32ErrorLogMessage
    {
    public:
        Win32ErrorLogMessage(const char* file, int line, LogSeverity severity,
            SystemErrorCode err, const char* module);

        Win32ErrorLogMessage(const char* file, int line, LogSeverity severity,
            SystemErrorCode err);

        // Appends the error message before destructing the encapsulated class.
        ~Win32ErrorLogMessage();

        std::ostream& stream() { return log_message_.stream(); }

    private:
        SystemErrorCode err_;
        // 定义错误代码的模块名(可选)
        const char* module_;
        LogMessage log_message_;

        DISALLOW_COPY_AND_ASSIGN(Win32ErrorLogMessage);
    };

    // 显式关闭日志文件.
    // 注意: 日志文件会根据日志记录的需要被打开, 所以无法保证CloseLogFile后
    // 文件一定是关闭状态.
    void CloseLogFile();

    // 直接写日志操作.
    void RawLog(int level, const char* message);

#define RAW_LOG(level, message) base::RawLog(base::LOG_ ## level, message)

#define RAW_CHECK(condition) \
    do \
    { \
        if(!(condition)) \
            base::RawLog(base::LOG_FATAL, "Check failed: " #condition "\n"); \
    } while(0)

} //namespace base

// 通过流的方式为日志输出提供便利.
// 设计允许输出Unicode字符串到日志文件. 这种方式相对来说会慢一些, 一般最好不要使用.
// 宽字符被转化为UTF-8通过操作符重载.
std::ostream& operator<<(std::ostream& out, const wchar_t* wstr);
inline std::ostream& operator<<(std::ostream& out, const std::wstring& wstr)
{
    return out << wstr.c_str();
}

// NOTIMPLEMENTED()宏实现
//
// 宏的实现依赖NOTIMPLEMENTED_POLICY的定义
//   0 -- 不实现(交由编译器)
//   1 -- 编译时警告
//   2 -- 编译时失败
//   3 -- 运行时失败(DCHECK)
//   4 -- (缺省)在运行时调用LOG(ERROR)
//   5 -- 在运行时调用LOG(ERROR), 只出现一次
#ifndef NOTIMPLEMENTED_POLICY
// 缺省策略: LOG(ERROR)
#define NOTIMPLEMENTED_POLICY 4
#endif

#define NOTIMPLEMENTED_MSG "NOT IMPLEMENTED"

#if NOTIMPLEMENTED_POLICY == 0
#define NOTIMPLEMENTED()
#elif NOTIMPLEMENTED_POLICY == 1
// TODO, figure out how to generate a warning
#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 2
#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 3
#define NOTIMPLEMENTED() NOTREACHED()
#elif NOTIMPLEMENTED_POLICY == 4
#define NOTIMPLEMENTED() LOG(ERROR) << NOTIMPLEMENTED_MSG
#elif NOTIMPLEMENTED_POLICY == 5
#define NOTIMPLEMENTED()\
    do \
    { \
        static int count = 0; \
        LOG_IF(ERROR, 0==count++) << NOTIMPLEMENTED_MSG; \
    } while(0)
#endif

namespace base
{
    class StringPiece;
}
// 允许StringPiece用于日志.
extern std::ostream& operator<<(std::ostream& o, const base::StringPiece& piece);

#endif //__base_logging_h__