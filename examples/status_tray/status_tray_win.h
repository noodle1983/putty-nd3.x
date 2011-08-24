
#ifndef __status_tray_win_h__
#define __status_tray_win_h__

#pragma once

#include <windows.h>

#include "status_tray.h"

class StatusTrayWin : public StatusTray
{
public:
    StatusTrayWin();
    ~StatusTrayWin();

    // Exposed for testing.
    LRESULT CALLBACK WndProc(HWND hwnd,
        UINT message,
        WPARAM wparam,
        LPARAM lparam);

protected:
    // Overriden from StatusTray:
    virtual StatusIcon* CreatePlatformStatusIcon();

private:
    // Static callback invoked when a message comes in to our messaging window.
    static LRESULT CALLBACK WndProcStatic(HWND hwnd,
        UINT message,
        WPARAM wparam,
        LPARAM lparam);

    // The unique icon ID we will assign to the next icon.
    UINT next_icon_id_;

    // The window used for processing events.
    HWND window_;

    // The message ID of the "TaskbarCreated" message, sent to us when we need to
    // reset our status icons.
    UINT taskbar_created_message_;

    DISALLOW_COPY_AND_ASSIGN(StatusTrayWin);
};

#endif //__status_tray_win_h__