
#ifndef __ui_base_shell_h__
#define __ui_base_shell_h__

#pragma once

#include <windows.h>

#include "base/string16.h"

class FilePath;

namespace ui
{
    namespace win
    {

        // Open or run a file via the Windows shell. In the event that there is no
        // default application registered for the file specified by 'full_path',
        // ask the user, via the Windows "Open With" dialog.
        // Returns 'true' on successful open, 'false' otherwise.
        bool OpenItemViaShell(const FilePath& full_path);

        // The download manager now writes the alternate data stream with the
        // zone on all downloads. This function is equivalent to OpenItemViaShell
        // without showing the zone warning dialog.
        bool OpenItemViaShellNoZoneCheck(const FilePath& full_path);

        // Ask the user, via the Windows "Open With" dialog, for an application to use
        // to open the file specified by 'full_path'.
        // Returns 'true' on successful open, 'false' otherwise.
        bool OpenItemWithExternalApp(const string16& full_path);

    } //namespace win
} //namespace ui

#endif //__ui_base_shell_h__