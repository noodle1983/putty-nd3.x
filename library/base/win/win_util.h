
#ifndef __base_win_win_util_h__
#define __base_win_win_util_h__

#pragma once

#include <windows.h>

#include <string>

namespace base
{

    void GetNonClientMetrics(NONCLIENTMETRICS* metrics);

    // 返回当前用户的sid字符串.
    bool GetUserSidString(std::wstring* user_sid);

    // 返回shift键当前是否被按下.
    bool IsShiftPressed();

    // 返回ctrl键当前是否被按下.
    bool IsCtrlPressed();

    // 返回alt键当前是否被按下.
    bool IsAltPressed();

    // 如果用户帐户控制(UAC)已经通过注册表EnableLUA键值被禁用返回false. 如果启用
    // 返回true.
    // 注意: EnableLUA键值在Windows XP上是被忽略的, 但可能存在并被设置成0(禁用UAC),
    // 此时会返回false. 调用前需要检查操作系统为Vista或者以后的版本.
    bool UserAccountControlIsEnabled();

    // 使用FormatMessage() API生成字符串, 使用Windows缺省的消息编译资源;
    // 忽略%占位符.
    std::wstring FormatMessage(unsigned messageid);

    // 使用GetLastError()生成可读的消息字符串.
    std::wstring FormatLastWin32Error();

} //namespace base

#endif //__base_win_win_util_h__