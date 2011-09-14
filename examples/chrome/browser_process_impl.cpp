
#include "browser_process_impl.h"

#include <map>
#include <set>
#include <vector>

#include "base/command_line.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/synchronization/waitable_event.h"
#include "base/task.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"

#include "ui_base/clipboard/clipboard.h"

#include "view/focus/view_storage.h"

#include "../status_tray/status_tray.h"

#include "browser_list.h"
#include "profile.h"
#include "profile_manager.h"

// How often to check if the persistent instance of Chrome needs to restart
// to install an update.
static const int kUpdateCheckIntervalHours = 6;

BrowserProcessImpl::BrowserProcessImpl()
: created_resource_dispatcher_host_(false),
created_metrics_service_(false),
created_io_thread_(false),
created_file_thread_(false),
created_db_thread_(false),
created_process_launcher_thread_(false),
created_cache_thread_(false),
created_watchdog_thread_(false),
created_profile_manager_(false),
created_local_state_(false),
created_icon_manager_(false),
created_sidebar_manager_(false),
created_notification_ui_manager_(false),
module_ref_count_(0),
did_start_(false),
checked_for_new_frames_(false),
using_new_frames_(false)
{
    g_browser_process = this;
    clipboard_.reset(new ui::Clipboard);
}

BrowserProcessImpl::~BrowserProcessImpl()
{
    // Delete the AutomationProviderList before NotificationService,
    // since it may try to unregister notifications
    // Both NotificationService and AutomationProvider are singleton instances in
    // the BrowserProcess. Since AutomationProvider may have some active
    // notification observers, it is essential that it gets destroyed before the
    // NotificationService. NotificationService won't be destroyed until after
    // this destructor is run.
    //automation_provider_list_.reset();

    // We need to destroy the MetricsService, GoogleURLTracker,
    // IntranetRedirectDetector, and SafeBrowsing ClientSideDetectionService
    // before the io_thread_ gets destroyed, since their destructors can call the
    // URLFetcher destructor, which does a PostDelayedTask operation on the IO
    // thread. (The IO thread will handle that URLFetcher operation before going
    // away.)
    //metrics_service_.reset();

    // Need to clear the desktop notification balloons before the io_thread_ and
    // before the profiles, since if there are any still showing we will access
    // those things during teardown.
    //notification_ui_manager_.reset();

    // Need to clear profiles (download managers) before the io_thread_.
    profile_manager_.reset();

    //if(resource_dispatcher_host_.get())
    //{
    //    // Cancel pending requests and prevent new requests.
    //    resource_dispatcher_host()->Shutdown();
    //}

    // Destroying the GpuProcessHostUIShims on the UI thread posts a task to
    // delete related objects on the GPU thread. This must be done before
    // stopping the GPU thread. The GPU thread will close IPC channels to renderer
    // processes so this has to happen before stopping the IO thread.
    //GpuProcessHostUIShim::DestroyAll();

    // Stop the watchdog thread before stopping other threads.
    //watchdog_thread_.reset();

    // Need to stop io_thread_ before resource_dispatcher_host_, since
    // io_thread_ may still deref ResourceDispatcherHost and handle resource
    // request before going away.
    //io_thread_.reset();

    // The IO thread was the only user of this thread.
    cache_thread_.reset();

    // Stop the process launcher thread after the IO thread, in case the IO thread
    // posted a task to terminate a process on the process launcher thread.
    process_launcher_thread_.reset();

    // Clean up state that lives on the file_thread_ before it goes away.
    //if(resource_dispatcher_host_.get())
    //{
    //    resource_dispatcher_host()->download_file_manager()->Shutdown();
    //    resource_dispatcher_host()->save_file_manager()->Shutdown();
    //}

    // Need to stop the file_thread_ here to force it to process messages in its
    // message loop from the previous call to shutdown the DownloadFileManager,
    // SaveFileManager and SessionService.
    file_thread_.reset();

    // With the file_thread_ flushed, we can release any icon resources.
    //icon_manager_.reset();

    // Need to destroy ResourceDispatcherHost before PluginService and
    // SafeBrowsingService, since it caches a pointer to it. This also
    // causes the webkit thread to terminate.
    //resource_dispatcher_host_.reset();

    // Destroy TabCloseableStateWatcher before NotificationService since the
    // former registers for notifications.
    //tab_closeable_state_watcher_.reset();

    g_browser_process = NULL;
}

// Send a QuitTask to the given MessageLoop.
static void PostQuit(MessageLoop* message_loop)
{
    message_loop->PostTask(new MessageLoop::QuitTask());
}

unsigned int BrowserProcessImpl::AddRefModule()
{
    DCHECK(CalledOnValidThread());
    did_start_ = true;
    module_ref_count_++;
    return module_ref_count_;
}

unsigned int BrowserProcessImpl::ReleaseModule()
{
    DCHECK(CalledOnValidThread());
    DCHECK_NE(0u, module_ref_count_);
    module_ref_count_--;
    if(0 == module_ref_count_)
    {
        // Allow UI and IO threads to do blocking IO on shutdown, since we do a lot
        // of it on shutdown for valid reasons.
        base::ThreadRestrictions::SetIOAllowed(true);
        //io_thread()->message_loop()->PostTask(
        //    NewRunnableFunction(&base::ThreadRestrictions::SetIOAllowed, true));
        //MessageLoop::current()->PostTask(
        //    NewRunnableFunction(DidEndMainMessageLoop));
        MessageLoop::current()->Quit();
    }
    return module_ref_count_;
}

void BrowserProcessImpl::EndSession()
{
    // Mark all the profiles as clean.
    ProfileManager* pm = profile_manager();
    std::vector<Profile*> profiles(pm->GetLoadedProfiles());
    for(size_t i=0; i<profiles.size(); ++i)
    {
        profiles[i]->MarkAsCleanShutdown();
    }

    // Tell the metrics service it was cleanly shutdown.
    //MetricsService* metrics = g_browser_process->metrics_service();
    //if(metrics && local_state())
    //{
    //    metrics->RecordStartOfSessionEnd();

    //    // MetricsService lazily writes to prefs, force it to write now.
    //    local_state()->SavePersistentPrefs();
    //}

    // We must write that the profile and metrics service shutdown cleanly,
    // otherwise on startup we'll think we crashed. So we block until done and
    // then proceed with normal shutdown.
    //BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
    //    NewRunnableFunction(PostQuit, MessageLoop::current()));
    MessageLoop::current()->Run();
}

//ResourceDispatcherHost* BrowserProcessImpl::resource_dispatcher_host()
//{
//    DCHECK(CalledOnValidThread());
//    if(!created_resource_dispatcher_host_)
//    {
//        CreateResourceDispatcherHost();
//    }
//    return resource_dispatcher_host_.get();
//}

//MetricsService* BrowserProcessImpl::metrics_service()
//{
//    DCHECK(CalledOnValidThread());
//    if(!created_metrics_service_)
//    {
//        CreateMetricsService();
//    }
//    return metrics_service_.get();
//}

//IOThread* BrowserProcessImpl::io_thread()
//{
//    DCHECK(CalledOnValidThread());
//    if(!created_io_thread_)
//    {
//        CreateIOThread();
//    }
//    return io_thread_.get();
//}

base::Thread* BrowserProcessImpl::file_thread()
{
    DCHECK(CalledOnValidThread());
    if(!created_file_thread_)
    {
        CreateFileThread();
    }
    return file_thread_.get();
}

base::Thread* BrowserProcessImpl::db_thread()
{
    DCHECK(CalledOnValidThread());
    if(!created_db_thread_)
    {
        CreateDBThread();
    }
    return db_thread_.get();
}

base::Thread* BrowserProcessImpl::process_launcher_thread()
{
    DCHECK(CalledOnValidThread());
    if(!created_process_launcher_thread_)
    {
        CreateProcessLauncherThread();
    }
    return process_launcher_thread_.get();
}

base::Thread* BrowserProcessImpl::cache_thread()
{
    DCHECK(CalledOnValidThread());
    if(!created_cache_thread_)
    {
        CreateCacheThread();
    }
    return cache_thread_.get();
}

//WatchDogThread* BrowserProcessImpl::watchdog_thread()
//{
//    DCHECK(CalledOnValidThread());
//    if(!created_watchdog_thread_)
//    {
//        CreateWatchdogThread();
//    }
//    DCHECK(watchdog_thread_.get() != NULL);
//    return watchdog_thread_.get();
//}

ProfileManager* BrowserProcessImpl::profile_manager()
{
    DCHECK(CalledOnValidThread());
    if(!created_profile_manager_)
    {
        CreateProfileManager();
    }
    return profile_manager_.get();
}

PrefService* BrowserProcessImpl::local_state()
{
    //DCHECK(CalledOnValidThread());
    //if(!created_local_state_)
    //{
    //    CreateLocalState();
    //}
    //return local_state_.get();
    return NULL;
}

SidebarManager* BrowserProcessImpl::sidebar_manager()
{
    //DCHECK(CalledOnValidThread());
    //if(!created_sidebar_manager_)
    //{
    //    CreateSidebarManager();
    //}
    //return sidebar_manager_.get();
    return NULL;
}

ui::Clipboard* BrowserProcessImpl::clipboard()
{
    DCHECK(CalledOnValidThread());
    return clipboard_.get();
}

//NotificationUIManager* BrowserProcessImpl::notification_ui_manager()
//{
//    DCHECK(CalledOnValidThread());
//    if(!created_notification_ui_manager_)
//    {
//        CreateNotificationUIManager();
//    }
//    return notification_ui_manager_.get();
//}

//IconManager* BrowserProcessImpl::icon_manager()
//{
//    DCHECK(CalledOnValidThread());
//    if(!created_icon_manager_)
//    {
//        CreateIconManager();
//    }
//    return icon_manager_.get();
//}
//
//ThumbnailGenerator* BrowserProcessImpl::GetThumbnailGenerator()
//{
//    return &thumbnail_generator_;
//}

//AutomationProviderList* BrowserProcessImpl::InitAutomationProviderList()
//{
//    DCHECK(CalledOnValidThread());
//    if(automation_provider_list_.get() == NULL)
//    {
//        automation_provider_list_.reset(AutomationProviderList::GetInstance());
//    }
//    return automation_provider_list_.get();
//}

bool BrowserProcessImpl::IsShuttingDown()
{
    DCHECK(CalledOnValidThread());
    return did_start_ && 0 == module_ref_count_;
}

const std::string& BrowserProcessImpl::GetApplicationLocale()
{
    DCHECK(!locale_.empty());
    return locale_;
}

void BrowserProcessImpl::SetApplicationLocale(const std::string& locale)
{
    locale_ = locale;
    //extension_l10n_util::SetProcessLocale(locale);
}

TabCloseableStateWatcher* BrowserProcessImpl::tab_closeable_state_watcher()
{
    //DCHECK(CalledOnValidThread());
    //if(!tab_closeable_state_watcher_.get())
    //{
    //    CreateTabCloseableStateWatcher();
    //}
    //return tab_closeable_state_watcher_.get();
    return NULL;
}

StatusTray* BrowserProcessImpl::status_tray()
{
    DCHECK(CalledOnValidThread());
    if(!status_tray_.get())
    {
        CreateStatusTray();
    }
    return status_tray_.get();
}

void BrowserProcessImpl::StartAutoupdateTimer()
{
    autoupdate_timer_.Start(
        base::TimeDelta::FromHours(kUpdateCheckIntervalHours),
        this,
        &BrowserProcessImpl::OnAutoupdateTimer);
}

void BrowserProcessImpl::CreateResourceDispatcherHost()
{
    //DCHECK(!created_resource_dispatcher_host_ &&
    //    resource_dispatcher_host_.get()==NULL);
    //created_resource_dispatcher_host_ = true;

    //// UserScriptListener will delete itself.
    //ResourceQueue::DelegateSet resource_queue_delegates;
    //resource_queue_delegates.insert(new UserScriptListener());

    //resource_dispatcher_host_.reset(
    //    new ResourceDispatcherHost(resource_queue_delegates));
    //resource_dispatcher_host_->Initialize();

    //resource_dispatcher_host_delegate_.reset(
    //    new ChromeResourceDispatcherHostDelegate(resource_dispatcher_host_.get(),
    //    prerender_tracker()));
    //resource_dispatcher_host_->set_delegate(
    //    resource_dispatcher_host_delegate_.get());

    //pref_change_registrar_.Add(prefs::kAllowCrossOriginAuthPrompt, this);
    //ApplyAllowCrossOriginAuthPromptPolicy();
}

void BrowserProcessImpl::CreateMetricsService()
{
    //DCHECK(!created_metrics_service_ && metrics_service_.get() == NULL);
    //created_metrics_service_ = true;

    //metrics_service_.reset(new MetricsService);
}

void BrowserProcessImpl::CreateIOThread()
{
    //DCHECK(!created_io_thread_ && io_thread_.get()==NULL);
    //created_io_thread_ = true;

    //// Prior to starting the io thread, we create the plugin service as
    //// it is predominantly used from the io thread, but must be created
    //// on the main thread. The service ctor is inexpensive and does not
    //// invoke the io_thread() accessor.
    //PluginService::GetInstance();

    //// Add the Chrome specific plugins.
    //chrome::RegisterInternalDefaultPlugin();

    //// Register the internal Flash if available.
    //FilePath path;
    //if(!CommandLine::ForCurrentProcess()->HasSwitch(
    //    switches::kDisableInternalFlash) &&
    //    PathService::Get(chrome::FILE_FLASH_PLUGIN, &path))
    //{
    //    webkit::npapi::PluginList::Singleton()->AddExtraPluginPath(path);
    //}

    //scoped_ptr<IOThread> thread(new IOThread(
    //    local_state(), net_log_.get(), extension_event_router_forwarder_.get()));
    //base::Thread::Options options;
    //options.message_loop_type = MessageLoop::TYPE_IO;
    //if(!thread->StartWithOptions(options))
    //{
    //    return;
    //}
    //io_thread_.swap(thread);
}

void BrowserProcessImpl::CreateFileThread()
{
    //DCHECK(!created_file_thread_ && file_thread_.get() == NULL);
    //created_file_thread_ = true;

    //scoped_ptr<base::Thread> thread(
    //    new BrowserProcessSubThread(BrowserThread::FILE));
    //base::Thread::Options options;
    //// On Windows, the FILE thread needs to be have a UI message loop which pumps
    //// messages in such a way that Google Update can communicate back to us.
    //options.message_loop_type = MessageLoop::TYPE_UI;
    //if(!thread->StartWithOptions(options))
    //{
    //    return;
    //}
    //file_thread_.swap(thread);
}

void BrowserProcessImpl::CreateDBThread()
{
    //DCHECK(!created_db_thread_ && db_thread_.get() == NULL);
    //created_db_thread_ = true;

    //scoped_ptr<base::Thread> thread(
    //    new BrowserProcessSubThread(BrowserThread::DB));
    //if(!thread->Start())
    //{
    //    return;
    //}
    //db_thread_.swap(thread);
}

void BrowserProcessImpl::CreateProcessLauncherThread()
{
    //DCHECK(!created_process_launcher_thread_ && !process_launcher_thread_.get());
    //created_process_launcher_thread_ = true;

    //scoped_ptr<base::Thread> thread(
    //    new BrowserProcessSubThread(BrowserThread::PROCESS_LAUNCHER));
    //if(!thread->Start())
    //{
    //    return;
    //}
    //process_launcher_thread_.swap(thread);
}

void BrowserProcessImpl::CreateCacheThread()
{
    //DCHECK(!created_cache_thread_ && !cache_thread_.get());
    //created_cache_thread_ = true;

    //scoped_ptr<base::Thread> thread(
    //    new BrowserThread(BrowserThread::CACHE));
    //base::Thread::Options options;
    //options.message_loop_type = MessageLoop::TYPE_IO;
    //if(!thread->StartWithOptions(options))
    //{
    //    return;
    //}
    //cache_thread_.swap(thread);
}

void BrowserProcessImpl::CreateWatchdogThread()
{
    //DCHECK(!created_watchdog_thread_ && watchdog_thread_.get()==NULL);
    //created_watchdog_thread_ = true;

    //scoped_ptr<WatchDogThread> thread(new WatchDogThread());
    //if(!thread->Start())
    //{
    //    return;
    //}
    //watchdog_thread_.swap(thread);
}

void BrowserProcessImpl::CreateProfileManager()
{
    DCHECK(!created_profile_manager_ && profile_manager_.get() == NULL);
    created_profile_manager_ = true;

    FilePath user_data_dir;
    //PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
    profile_manager_.reset(new ProfileManager(user_data_dir));
}

void BrowserProcessImpl::CreateLocalState()
{
    //DCHECK(!created_local_state_ && local_state_.get() == NULL);
    //created_local_state_ = true;

    //FilePath local_state_path;
    //PathService::Get(chrome::FILE_LOCAL_STATE, &local_state_path);
    //local_state_.reset(
    //    PrefService::CreatePrefService(local_state_path, NULL, false));

    //// Initialize the prefs of the local state.
    //browser::RegisterLocalState(local_state_.get());

    //pref_change_registrar_.Init(local_state_.get());

    //// Initialize the notification for the default browser setting policy.
    //local_state_->RegisterBooleanPref(prefs::kDefaultBrowserSettingEnabled,
    //    false);
    //if(local_state_->IsManagedPreference(prefs::kDefaultBrowserSettingEnabled))
    //{
    //    if(local_state_->GetBoolean(prefs::kDefaultBrowserSettingEnabled))
    //    {
    //        ShellIntegration::SetAsDefaultBrowser();
    //    }
    //}
    //pref_change_registrar_.Add(prefs::kDefaultBrowserSettingEnabled, this);

    //// Initialize the preference for the plugin finder policy.
    //// This preference is only needed on the IO thread so make it available there.
    //local_state_->RegisterBooleanPref(prefs::kDisablePluginFinder, false);
    //plugin_finder_disabled_pref_.Init(prefs::kDisablePluginFinder,
    //    local_state_.get(), NULL);
    //plugin_finder_disabled_pref_.MoveToThread(BrowserThread::IO);

    //// Initialize the disk cache location policy. This policy is not hot update-
    //// able so we need to have it when initializing the profiles.
    //local_state_->RegisterFilePathPref(prefs::kDiskCacheDir, FilePath());

    //// Another policy that needs to be defined before the net subsystem is
    //// initialized is MaxConnectionsPerProxy so we do it here.
    //local_state_->RegisterIntegerPref(prefs::kMaxConnectionsPerProxy,
    //    net::kDefaultMaxSocketsPerProxyServer);
    //int max_per_proxy = local_state_->GetInteger(prefs::kMaxConnectionsPerProxy);
    //net::ClientSocketPoolManager::set_max_sockets_per_proxy_server(
    //    std::max(std::min(max_per_proxy, 99),
    //    net::ClientSocketPoolManager::max_sockets_per_group()));

    //// This is observed by ChildProcessSecurityPolicy, which lives in content/
    //// though, so it can't register itself.
    //local_state_->RegisterListPref(prefs::kDisabledSchemes);
    //pref_change_registrar_.Add(prefs::kDisabledSchemes, this);
    //ApplyDisabledSchemesPolicy();
}

void BrowserProcessImpl::CreateIconManager()
{
    //DCHECK(!created_icon_manager_ && icon_manager_.get() == NULL);
    //created_icon_manager_ = true;
    //icon_manager_.reset(new IconManager);
}

void BrowserProcessImpl::CreateSidebarManager()
{
    //DCHECK(sidebar_manager_.get() == NULL);
    //created_sidebar_manager_ = true;
    //sidebar_manager_ = new SidebarManager();
}

void BrowserProcessImpl::CreateNotificationUIManager()
{
    //DCHECK(notification_ui_manager_.get() == NULL);
    //notification_ui_manager_.reset(NotificationUIManager::Create(local_state()));

    //created_notification_ui_manager_ = true;
}

void BrowserProcessImpl::CreateTabCloseableStateWatcher()
{
    //DCHECK(tab_closeable_state_watcher_.get() == NULL);
    //tab_closeable_state_watcher_.reset(TabCloseableStateWatcher::Create());
}

void BrowserProcessImpl::CreateStatusTray()
{
    DCHECK(status_tray_.get() == NULL);
    status_tray_.reset(StatusTray::Create());
}

void BrowserProcessImpl::ApplyDisabledSchemesPolicy()
{
    //std::set<std::string> schemes;
    //const ListValue* scheme_list = local_state_->GetList(prefs::kDisabledSchemes);
    //for(ListValue::const_iterator iter=scheme_list->begin();
    //    iter!=scheme_list->end(); ++iter)
    //{
    //    std::string scheme;
    //    if((*iter)->GetAsString(&scheme))
    //    {
    //        schemes.insert(scheme);
    //    }
    //}
    //ChildProcessSecurityPolicy::GetInstance()->RegisterDisabledSchemes(schemes);
}

void BrowserProcessImpl::ApplyAllowCrossOriginAuthPromptPolicy()
{
    //bool value = local_state()->GetBoolean(prefs::kAllowCrossOriginAuthPrompt);
    //resource_dispatcher_host()->set_allow_cross_origin_auth_prompt(value);
}

// The BrowserProcess object must outlive the file thread so we use traits
// which don't do any management.
DISABLE_RUNNABLE_METHOD_REFCOUNT(BrowserProcessImpl);

bool BrowserProcessImpl::CanAutorestartForUpdate() const
{
    // Check if browser is in the background and if it needs to be restarted to
    // apply a pending update.
    return BrowserList::size()==0 && BrowserList::WillKeepAlive()/* &&
        upgrade_util::IsUpdatePendingRestart()*/;
}

// Switches to add when auto-restarting Chrome.
//const char* const kSwitchesToAddOnAutorestart[] =
//{
//    switches::kNoStartupWindow
//};

void BrowserProcessImpl::RestartPersistentInstance()
{
    CommandLine* old_cl = CommandLine::ForCurrentProcess();
    scoped_ptr<CommandLine> new_cl(new CommandLine(old_cl->GetProgram()));

    std::map<std::string, CommandLine::StringType> switches =
        old_cl->GetSwitches();

    //switches::RemoveSwitchesForAutostart(&switches);

    // Append the rest of the switches (along with their values, if any)
    // to the new command line
    for(std::map<std::string, CommandLine::StringType>::const_iterator i=
        switches.begin(); i!=switches.end(); ++i)
    {
        CommandLine::StringType switch_value = i->second;
        if(switch_value.length() > 0)
        {
            new_cl->AppendSwitchNative(i->first, i->second);
        }
        else
        {
            new_cl->AppendSwitch(i->first);
        }
    }

    // Ensure that our desired switches are set on the new process.
    //for(size_t i=0; i<arraysize(kSwitchesToAddOnAutorestart); ++i)
    //{
    //    if(!new_cl->HasSwitch(kSwitchesToAddOnAutorestart[i]))
    //    {
    //        new_cl->AppendSwitch(kSwitchesToAddOnAutorestart[i]);
    //    }
    //}

    DLOG(WARNING) << "Shutting down current instance of the browser.";
    BrowserList::AttemptExit();

    // Transfer ownership to Upgrade.
    //upgrade_util::SetNewCommandLine(new_cl.release());
}

void BrowserProcessImpl::OnAutoupdateTimer()
{
    if(CanAutorestartForUpdate())
    {
        DLOG(WARNING) << "Detected update.  Restarting browser.";
        RestartPersistentInstance();
    }
}