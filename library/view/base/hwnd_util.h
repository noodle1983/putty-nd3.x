
#ifndef __view_hwnd_util_h__
#define __view_hwnd_util_h__

#pragma once

#include <windows.h>

#include <string>

namespace gfx
{
    class Size;
}

namespace view
{

    // GetClassNameW API的封装, 返回窗口类名. 空值表示获取类名失败.
    std::wstring GetClassName(HWND window);

    // 子类化窗口, 返回被替换的窗口过程.
    WNDPROC SetWindowProc(HWND hwnd, WNDPROC wndproc);

    // Get/SetWindowLong(..., GWLP_USERDATA, ...)的封装.
    // 返回设置前的用户数据.
    void* SetWindowUserData(HWND hwnd, void* user_data);
    void* GetWindowUserData(HWND hwnd);

    // Returns true if the specified window is the current active top window or one
    // of its children.
    bool DoesWindowBelongToActiveWindow(HWND window);

    // Sizes the window to have a client or window size (depending on the value of
    // |pref_is_client|) of pref, then centers the window over parent, ensuring the
    // window fits on screen.
    void CenterAndSizeWindow(HWND parent, HWND window,
        const gfx::Size& pref, bool pref_is_client);

} //namespace view

#endif //__view_hwnd_util_h__