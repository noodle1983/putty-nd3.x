
#ifndef __base_diagnose_debugger_h__
#define __base_diagnose_debugger_h__

#pragma once

namespace base
{

    // 启动系统已注册的JIT调试器并附加到指定进程.
    bool SpawnDebuggerOnProcess(unsigned process_id);

    // 等待wait_seconds秒以便调试器附加到当前进程. 当silent==false, 监测到
    // 调试器后会抛出异常.
    bool WaitForDebugger(int wait_seconds, bool silent);

    // 在调试器下运行?
    bool BeingDebugged();

    // 在调试器里中断, 前提是存在调试器.
    void BreakDebugger();

    // 仅在测试代码中用到. 用于控制在发生错误时是否显示对话框并进入调试器.
    // 一般在自动化测试的时候使用.
    void SetSuppressDebugUI(bool suppress);
    bool IsDebugUISuppressed();

} //namespace base

#endif //__base_diagnose_debugger_h__