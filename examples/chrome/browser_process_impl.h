
// When each service is created, we set a flag indicating this. At this point,
// the service initialization could fail or succeed. This allows us to remember
// if we tried to create a service, and not try creating it over and over if
// the creation failed.

#ifndef __browser_process_impl_h__
#define __browser_process_impl_h__

#pragma once

#include <string>

#include "base/basic_types.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/threading/non_thread_safe.h"
#include "base/timer.h"

#include "browser_process.h"

class FilePath;
class TabCloseableStateWatcher;

// Real implementation of BrowserProcess that creates and returns the services.
class BrowserProcessImpl : public BrowserProcess,
    public base::NonThreadSafe
{
public:
    explicit BrowserProcessImpl();
    virtual ~BrowserProcessImpl();

    virtual void EndSession();

    // BrowserProcess methods
    virtual base::Thread* file_thread();
    virtual base::Thread* db_thread();
    virtual base::Thread* process_launcher_thread();
    virtual base::Thread* cache_thread();
    virtual ProfileManager* profile_manager();
    virtual PrefService* local_state();
    virtual SidebarManager* sidebar_manager();
    virtual ui::Clipboard* clipboard();
    virtual unsigned int AddRefModule();
    virtual unsigned int ReleaseModule();
    virtual bool IsShuttingDown();
    virtual const std::string& GetApplicationLocale();
    virtual void SetApplicationLocale(const std::string& locale);
    virtual TabCloseableStateWatcher* tab_closeable_state_watcher();
    virtual StatusTray* status_tray();

    virtual void StartAutoupdateTimer();

private:
    void CreateResourceDispatcherHost();
    void CreateMetricsService();

    void CreateIOThread();
    static void CleanupOnIOThread();

    void CreateFileThread();
    void CreateDBThread();
    void CreateProcessLauncherThread();
    void CreateCacheThread();
    void CreateGpuThread();
    void CreateWatchdogThread();
    void CreateTemplateURLService();
    void CreateProfileManager();
    void CreateWebDataService();
    void CreateLocalState();
    void CreateViewedPageTracker();
    void CreateIconManager();
    void CreateSidebarManager();
    void CreateNotificationUIManager();
    void CreateStatusTrayManager();
    void CreateTabCloseableStateWatcher();
    void CreateStatusTray();

    void ApplyDisabledSchemesPolicy();
    void ApplyAllowCrossOriginAuthPromptPolicy();

    bool created_resource_dispatcher_host_;
    //scoped_ptr<ResourceDispatcherHost> resource_dispatcher_host_;

    bool created_metrics_service_;
    //scoped_ptr<MetricsService> metrics_service_;

    bool created_io_thread_;
    //scoped_ptr<IOThread> io_thread_;

    bool created_file_thread_;
    scoped_ptr<base::Thread> file_thread_;

    bool created_db_thread_;
    scoped_ptr<base::Thread> db_thread_;

    bool created_process_launcher_thread_;
    scoped_ptr<base::Thread> process_launcher_thread_;

    bool created_cache_thread_;
    scoped_ptr<base::Thread> cache_thread_;

    bool created_watchdog_thread_;
    //scoped_ptr<WatchDogThread> watchdog_thread_;

    bool created_profile_manager_;
    scoped_ptr<ProfileManager> profile_manager_;

    bool created_local_state_;
    //scoped_ptr<PrefService> local_state_;

    bool created_icon_manager_;
    //scoped_ptr<IconManager> icon_manager_;

    bool created_sidebar_manager_;
    //scoped_refptr<SidebarManager> sidebar_manager_;

    scoped_ptr<ui::Clipboard> clipboard_;

    // Manager for desktop notification UI.
    bool created_notification_ui_manager_;
    //scoped_ptr<NotificationUIManager> notification_ui_manager_;

    //scoped_ptr<AutomationProviderList> automation_provider_list_;

    //scoped_ptr<TabCloseableStateWatcher> tab_closeable_state_watcher_;

    scoped_ptr<StatusTray> status_tray_;

    unsigned int module_ref_count_;
    bool did_start_;

    std::string locale_;

    bool checked_for_new_frames_;
    bool using_new_frames_;

    // This service just sits around and makes thumbnails for tabs. It does
    // nothing in the constructor so we don't have to worry about lazy init.
    //ThumbnailGenerator thumbnail_generator_;

    // Download status updates (like a changing application icon on dock/taskbar)
    // are global per-application. DownloadStatusUpdater does no work in the ctor
    // so we don't have to worry about lazy initialization.
    //DownloadStatusUpdater download_status_updater_;

    // Ensures that the observers of plugin/print disable/enable state
    // notifications are properly added and removed.
    //PrefChangeRegistrar pref_change_registrar_;

    // Ordered before resource_dispatcher_host_delegate_ due to destruction
    // ordering.
    //scoped_ptr<prerender::PrerenderTracker> prerender_tracker_;

    //scoped_ptr<ChromeResourceDispatcherHostDelegate>
    //    resource_dispatcher_host_delegate_;

    //NotificationRegistrar notification_registrar_;

    //scoped_refptr<MHTMLGenerationManager> mhtml_generation_manager_;

    // Monitors the state of the 'DisablePluginFinder' policy.
    //BooleanPrefMember plugin_finder_disabled_pref_;

    base::RepeatingTimer<BrowserProcessImpl> autoupdate_timer_;

    // Gets called by autoupdate timer to see if browser needs restart and can be
    // restarted, and if that's the case, restarts the browser.
    void OnAutoupdateTimer();
    bool CanAutorestartForUpdate() const;
    void RestartPersistentInstance();

    DISALLOW_COPY_AND_ASSIGN(BrowserProcessImpl);
};

#endif //__browser_process_impl_h__