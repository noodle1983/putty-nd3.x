
#ifndef __browser_shutdown_h__
#define __browser_shutdown_h__

#pragma once

namespace browser_shutdown
{

    enum ShutdownType
    {
        // an uninitialized value
        NOT_VALID = 0,
        // the last browser window was closed
        WINDOW_CLOSE,
        // user clicked on the Exit menu item
        BROWSER_EXIT,
        // windows is logging off or shutting down
        END_SESSION
    };

    // Called when the browser starts shutting down so that we can measure shutdown
    // time.
    void OnShutdownStarting(ShutdownType type);

    // Get the current shutdown type.
    ShutdownType GetShutdownType();

    // Invoked in two ways:
    // . When the last browser has been deleted and the message loop has finished
    //   running.
    // . When ChromeFrame::EndSession is invoked and we need to do cleanup.
    //   NOTE: in this case the message loop is still running, but will die soon
    //         after this returns.
    void Shutdown();

    // There are various situations where the browser process should continue to
    // run after the last browser window has closed - the Mac always continues
    // running until the user explicitly quits, and on Windows/Linux the application
    // should not shutdown when the last browser window closes if there are any
    // BackgroundContents running.
    // When the user explicitly chooses to shutdown the app (via the "Exit" or
    // "Quit" menu items) BrowserList will call SetTryingToQuit() to tell itself to
    // initiate a shutdown when the last window closes.
    // If the quit is aborted, then the flag should be reset.

    // This is a low-level mutator; in general, don't call SetTryingToQuit(true),
    // except from appropriate places in BrowserList. To quit, use usual means,
    // e.g., using |chrome_browser_application_mac::Terminate()| on the Mac, or
    // |BrowserList::CloseAllWindowsAndExit()| on other platforms. To stop quitting,
    // use |chrome_browser_application_mac::CancelTerminate()| on the Mac; other
    // platforms can call SetTryingToQuit(false) directly.
    void SetTryingToQuit(bool quitting);

    // General accessor.
    bool IsTryingToQuit();

    // This is true on X during an END_SESSION, when we can no longer depend
    // on the X server to be running. As a result we don't explicitly close the
    // browser windows, which can lead to conditions which would fail checks.
    bool ShuttingDownWithoutClosingBrowsers();

} //namespace browser_shutdown

#endif //__browser_shutdown_h__