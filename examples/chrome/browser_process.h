
// This interface is for managing the global services of the application. Each
// service is lazily created when requested the first time. The service getters
// will return NULL if the service is not available, so callers must check for
// this condition.

#ifndef __browser_process_h__
#define __browser_process_h__

#pragma once

#include <string>
#include <vector>

#include "base/basic_types.h"
#include "base/memory/ref_counted.h"

class PrefService;
class Profile;
class ProfileManager;
class SidebarManager;
class StatusTray;
class TabCloseableStateWatcher;

namespace base
{
    class Thread;
}

namespace ui
{
    class Clipboard;
}

// NOT THREAD SAFE, call only from the main thread.
// These functions shouldn't return NULL unless otherwise noted.
class BrowserProcess
{
public:
    BrowserProcess();
    virtual ~BrowserProcess();

    // Invoked when the user is logging out/shutting down. When logging off we may
    // not have enough time to do a normal shutdown. This method is invoked prior
    // to normal shutdown and saves any state that must be saved before we are
    // continue shutdown.
    virtual void EndSession() = 0;

    // Services: any of these getters may return NULL
    virtual ProfileManager* profile_manager() = 0;
    virtual PrefService* local_state() = 0;
    virtual SidebarManager* sidebar_manager() = 0;
    virtual ui::Clipboard* clipboard() = 0;

    // Returns the thread that we perform random file operations on. For code
    // that wants to do I/O operations (not network requests or even file: URL
    // requests), this is the thread to use to avoid blocking the UI thread.
    // It might be nicer to have a thread pool for this kind of thing.
    virtual base::Thread* file_thread() = 0;

    // Returns the thread that is used for database operations such as the web
    // database. History has its own thread since it has much higher traffic.
    virtual base::Thread* db_thread() = 0;

    // Returns the thread that is used for background cache operations.
    virtual base::Thread* cache_thread() = 0;

    virtual unsigned int AddRefModule() = 0;
    virtual unsigned int ReleaseModule() = 0;

    virtual bool IsShuttingDown() = 0;

    // Returns the locale used by the application.
    virtual const std::string& GetApplicationLocale() = 0;
    virtual void SetApplicationLocale(const std::string& locale) = 0;

    // Returns the object that watches for changes in the closeable state of tab.
    virtual TabCloseableStateWatcher* tab_closeable_state_watcher() = 0;

    // Returns the StatusTray, which provides an API for displaying status icons
    // in the system status tray. Returns NULL if status icons are not supported
    // on this platform (or this is a unit test).
    virtual StatusTray* status_tray() = 0;

    // This will start a timer that, if Chrome is in persistent mode, will check
    // whether an update is available, and if that's the case, restart the
    // browser. Note that restart code will strip some of the command line keys
    // and all loose values from the cl this instance of Chrome was launched with,
    // and add the command line key that will force Chrome to start in the
    // background mode. For the full list of "blacklisted" keys, refer to
    // |kSwitchesToRemoveOnAutorestart| array in browser_process_impl.cc.
    virtual void StartAutoupdateTimer() = 0;

private:
    DISALLOW_COPY_AND_ASSIGN(BrowserProcess);
};

extern BrowserProcess* g_browser_process;

#endif //__browser_process_h__