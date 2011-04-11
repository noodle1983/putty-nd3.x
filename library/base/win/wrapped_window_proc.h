
#ifndef __base_win_wrapped_window_proc_h__
#define __base_win_wrapped_window_proc_h__

#pragma once

#include <windows.h>

namespace base
{

    // WindowProc的异常过滤器. 返回值决定如何处理异常, 遵守标准的SEH规则. 但是
    // 这个函数没有返回预期的EXCEPTION_EXECUTE_HANDLER类似信息, 因为一般我们不
    // 准备处理异常.
    typedef int (__cdecl *WinProcExceptionFilter)(EXCEPTION_POINTERS* info);

    // 设置过滤器处理WindowProc中的异常. 返回旧的异常过滤器.
    // 需要在创建窗口之前调用.
    WinProcExceptionFilter SetWinProcExceptionFilter(WinProcExceptionFilter filter);

    // 调用注册的异常过滤器.
    int CallExceptionFilter(EXCEPTION_POINTERS* info);

    // 封装为WindowProc提供标准的异常框架. 常用方法:
    //
    // LRESULT CALLBACK MyWinProc(HWND hwnd, UINT message,
    //                            WPARAM wparam, LPARAM lparam) {
    //   // Do Something.
    // }
    //
    // ...
    //
    //   WNDCLASSEX wc = { 0 };
    //   wc.lpfnWndProc = WrappedWindowProc<MyWinProc>;
    //   wc.lpszClassName = class_name;
    //   ...
    //   RegisterClassEx(&wc);
    //
    //   CreateWindowW(class_name, window_name, ...
    template<WNDPROC proc>
    LRESULT CALLBACK WrappedWindowProc(HWND hwnd, UINT message,
        WPARAM wparam, LPARAM lparam)
    {
        LRESULT rv = 0;
        __try
        {
            rv = proc(hwnd, message, wparam, lparam);
        }
        __except(CallExceptionFilter(GetExceptionInformation()))
        {
        }
        return rv;
    }

} //namespace base

#endif //__base_win_wrapped_window_proc_h__