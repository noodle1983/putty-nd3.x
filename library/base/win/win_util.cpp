
#include "win_util.h"

#include <sddl.h>

#include "base/memory/scoped_ptr.h"
#include "base/threading/thread_restrictions.h"
#include "registry.h"
#include "scoped_handle.h"
#include "windows_version.h"

namespace base
{
    namespace win
    {

#define SIZEOF_STRUCT_WITH_SPECIFIED_LAST_MEMBER(struct_name, member) \
    offsetof(struct_name, member) + \
    (sizeof static_cast<struct_name*>(NULL)->member)
#define NONCLIENTMETRICS_SIZE_PRE_VISTA \
    SIZEOF_STRUCT_WITH_SPECIFIED_LAST_MEMBER(NONCLIENTMETRICS, lfMessageFont)

        void GetNonClientMetrics(NONCLIENTMETRICS* metrics)
        {
            DCHECK(metrics);

            static const UINT SIZEOF_NONCLIENTMETRICS =
                (GetVersion()>=VERSION_VISTA) ?
                sizeof(NONCLIENTMETRICS) : NONCLIENTMETRICS_SIZE_PRE_VISTA;
            metrics->cbSize = SIZEOF_NONCLIENTMETRICS;
            const bool success = !!SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
                SIZEOF_NONCLIENTMETRICS, metrics, 0);
            DCHECK(success);
        }

        bool GetUserSidString(std::wstring* user_sid)
        {
            // 获取当前令牌.
            HANDLE token = NULL;
            if(!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token))
            {
                return false;
            }
            ScopedHandle token_scoped(token);

            DWORD size = sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE;
            scoped_array<BYTE> user_bytes(new BYTE[size]);
            TOKEN_USER* user = reinterpret_cast<TOKEN_USER*>(user_bytes.get());

            if(!::GetTokenInformation(token, TokenUser, user, size, &size))
            {
                return false;
            }

            if(!user->User.Sid)
            {
                return false;
            }

            // 数据转换成一个字符串.
            wchar_t* sid_string;
            if(!::ConvertSidToStringSid(user->User.Sid, &sid_string))
            {
                return false;
            }

            *user_sid = sid_string;

            ::LocalFree(sid_string);

            return true;
        }

        bool IsShiftPressed()
        {
            return (::GetKeyState(VK_SHIFT)&0x8000) == 0x8000;
        }

        bool IsCtrlPressed()
        {
            return (::GetKeyState(VK_CONTROL)&0x8000) == 0x8000;
        }

        bool IsAltPressed()
        {
            return (::GetKeyState(VK_MENU)&0x8000) == 0x8000;
        }

        bool UserAccountControlIsEnabled()
        {
            // 如果Windows退出写磁盘的时候会比较慢. 应该只读取一次, 然后监视键的变化,
            // 最好是在文件线程.
            //   http://code.google.com/p/chromium/issues/detail?id=61644
            ThreadRestrictions::ScopedAllowIO allow_io;

            RegKey key(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
                KEY_READ);
            DWORD uac_enabled;
            if(key.ReadValueDW(L"EnableLUA", &uac_enabled) != ERROR_SUCCESS)
            {
                return true;
            }
            // 用户可以设置EnableLUA键为任意值, 比如2, Vista认为这种情况是启动UAC,
            // 所以我们只需要确保是否为非0.
            return (uac_enabled != 0);
        }

        static const char16 kAutoRunKeyPath[] =
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

        bool AddCommandToAutoRun(HKEY root_key, const string16& name,
            const string16& command)
        {
            RegKey autorun_key(root_key, kAutoRunKeyPath, KEY_SET_VALUE);
            return (autorun_key.WriteValue(name.c_str(), command.c_str()) ==
                ERROR_SUCCESS);
        }

        bool RemoveCommandFromAutoRun(HKEY root_key, const string16& name)
        {
            RegKey autorun_key(root_key, kAutoRunKeyPath, KEY_SET_VALUE);
            return (autorun_key.DeleteValue(name.c_str()) == ERROR_SUCCESS);
        }

        bool ReadCommandFromAutoRun(HKEY root_key, const string16& name,
            string16* command)
        {
            RegKey autorun_key(root_key, kAutoRunKeyPath, KEY_QUERY_VALUE);
            return (autorun_key.ReadValue(name.c_str(), command) ==
                ERROR_SUCCESS);
        }

    } //namespace win
} //namespace base