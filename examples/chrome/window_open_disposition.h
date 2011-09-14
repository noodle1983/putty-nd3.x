
#ifndef __window_open_disposition_h__
#define __window_open_disposition_h__

#pragma once

enum WindowOpenDisposition
{
    SUPPRESS_OPEN,
    CURRENT_TAB,
    // Indicates that only one tab with the url should exist in the same window.
    SINGLETON_TAB,
    NEW_FOREGROUND_TAB,
    NEW_BACKGROUND_TAB,
    NEW_POPUP,
    NEW_WINDOW,
    SAVE_TO_DISK,
    IGNORE_ACTION
};

#endif //__window_open_disposition_h__