
#include "shell.h"

#include "base/file_path.h"

namespace ui
{
    namespace win
    {

        // Open an item via a shell execute command. Error code checking and casting
        // explanation: http://msdn2.microsoft.com/en-us/library/ms647732.aspx
        bool OpenItemViaShell(const FilePath& full_path)
        {
            HINSTANCE h = ::ShellExecuteW(
                NULL, NULL, full_path.value().c_str(), NULL,
                full_path.DirName().value().c_str(), SW_SHOWNORMAL);

            LONG_PTR error = reinterpret_cast<LONG_PTR>(h);
            if(error > 32)
            {
                return true;
            }

            if((error == SE_ERR_NOASSOC))
            {
                return OpenItemWithExternalApp(full_path.value());
            }

            return false;
        }

        bool OpenItemViaShellNoZoneCheck(const FilePath& full_path)
        {
            SHELLEXECUTEINFO sei = { sizeof(sei) };
            sei.fMask = SEE_MASK_NOZONECHECKS | SEE_MASK_FLAG_DDEWAIT;
            sei.nShow = SW_SHOWNORMAL;
            sei.lpVerb = NULL;
            sei.lpFile = full_path.value().c_str();
            if(::ShellExecuteExW(&sei))
            {
                return true;
            }
            LONG_PTR error = reinterpret_cast<LONG_PTR>(sei.hInstApp);
            if((error == SE_ERR_NOASSOC))
            {
                return OpenItemWithExternalApp(full_path.value());
            }
            return false;
        }

        // Show the Windows "Open With" dialog box to ask the user to pick an app to
        // open the file with.
        bool OpenItemWithExternalApp(const string16& full_path)
        {
            SHELLEXECUTEINFO sei = { sizeof(sei) };
            sei.fMask = SEE_MASK_FLAG_DDEWAIT;
            sei.nShow = SW_SHOWNORMAL;
            sei.lpVerb = L"openas";
            sei.lpFile = full_path.c_str();
            return (TRUE == ::ShellExecuteExW(&sei));
        }

    } //namespace win
} //namespace ui