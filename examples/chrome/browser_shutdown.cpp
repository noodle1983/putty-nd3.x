
#include "browser_shutdown.h"

#include "base/threading/thread_restrictions.h"
#include "base/time.h"

namespace browser_shutdown
{

    // Whether the browser is trying to quit (e.g., Quit chosen from menu).
    bool g_trying_to_quit = false;

    base::Time shutdown_started_;
    ShutdownType shutdown_type_ = NOT_VALID;

    ShutdownType GetShutdownType()
    {
        return shutdown_type_;
    }

    void OnShutdownStarting(ShutdownType type)
    {
        if(shutdown_type_ != NOT_VALID)
        {
            return;
        }

        shutdown_type_ = type;
        // For now, we're only counting the number of renderer processes
        // since we can't safely count the number of plugin processes from this
        // thread, and we'd really like to avoid anything which might add further
        // delays to shutdown time.
        shutdown_started_ = base::Time::Now();

        // Call FastShutdown on all of the RenderProcessHosts.  This will be
        // a no-op in some cases, so we still need to go through the normal
        // shutdown path for the ones that didn't exit here.
        //shutdown_num_processes_ = 0;
        //shutdown_num_processes_slow_ = 0;
        //for(RenderProcessHost::iterator i(RenderProcessHost::AllHostsIterator());
        //    !i.IsAtEnd(); i.Advance())
        //{
        //    ++shutdown_num_processes_;
        //    if(!i.GetCurrentValue()->FastShutdownIfPossible())
        //    {
        //        ++shutdown_num_processes_slow_;
        //    }
        //}
    }

    void Shutdown()
    {
        // During shutdown we will end up some blocking operations.  But the
        // work needs to get done and we're going to wait for them no matter
        // what thread they're on, so don't worry about it slowing down
        // shutdown.
        base::ThreadRestrictions::SetIOAllowed(true);

        // Shutdown the IPC channel to the service processes.
        //ServiceProcessControl::GetInstance()->Disconnect();

        // WARNING: During logoff/shutdown (WM_ENDSESSION) we may not have enough
        // time to get here. If you have something that *must* happen on end session,
        // consider putting it in BrowserProcessImpl::EndSession.
        //PrefService* prefs = g_browser_process->local_state();
        //ProfileManager* profile_manager = g_browser_process->profile_manager();
        //PrefService* user_prefs = profile_manager->GetDefaultProfile()->GetPrefs();

        //chrome_browser_net::SavePredictorStateForNextStartupAndTrim(user_prefs);

        //MetricsService* metrics = g_browser_process->metrics_service();
        //if(metrics)
        //{
        //    metrics->RecordCompletedSessionEnd();
        //}

        //if(shutdown_type_>NOT_VALID && shutdown_num_processes_>0)
        //{
        //    // Record the shutdown info so that we can put it into a histogram at next
        //    // startup.
        //    prefs->SetInteger(prefs::kShutdownType, shutdown_type_);
        //    prefs->SetInteger(prefs::kShutdownNumProcesses, shutdown_num_processes_);
        //    prefs->SetInteger(prefs::kShutdownNumProcessesSlow,
        //        shutdown_num_processes_slow_);
        //}

        //// Check local state for the restart flag so we can restart the session below.
        //bool restart_last_session = false;
        //if(prefs->HasPrefPath(prefs::kRestartLastSessionOnShutdown))
        //{
        //    restart_last_session =
        //        prefs->GetBoolean(prefs::kRestartLastSessionOnShutdown);
        //    prefs->ClearPref(prefs::kRestartLastSessionOnShutdown);
        //}

        //prefs->SavePersistentPrefs();

        //// Cleanup any statics created by RLZ. Must be done before NotificationService
        //// is destroyed.
        //RLZTracker::CleanupRlz();

        //// The jank'o'meter requires that the browser process has been destroyed
        //// before calling UninstallJankometer().
        //delete g_browser_process;
        //g_browser_process = NULL;

        //// Uninstall Jank-O-Meter here after the IO thread is no longer running.
        //UninstallJankometer();

        //if(delete_resources_on_shutdown)
        //{
        //    ResourceBundle::CleanupSharedInstance();
        //}

        //if(!browser_util::IsBrowserAlreadyRunning() &&
        //    shutdown_type_!=browser_shutdown::END_SESSION)
        //{
        //    upgrade_util::SwapNewChromeExeIfPresent();
        //}

        //if(restart_last_session)
        //{
        //    // Make sure to relaunch the browser with the original command line plus
        //    // the Restore Last Session flag. Note that Chrome can be launched (ie.
        //    // through ShellExecute on Windows) with a switch argument terminator at
        //    // the end (double dash, as described in b/1366444) plus a URL,
        //    // which prevents us from appending to the command line directly (issue
        //    // 46182). We therefore use GetSwitches to copy the command line (it stops
        //    // at the switch argument terminator).
        //    CommandLine old_cl(*CommandLine::ForCurrentProcess());
        //    scoped_ptr<CommandLine> new_cl(new CommandLine(old_cl.GetProgram()));
        //    std::map<std::string, CommandLine::StringType> switches =
        //        old_cl.GetSwitches();
        //    // Remove the switches that shouldn't persist across restart.
        //    about_flags::RemoveFlagsSwitches(&switches);
        //    switches::RemoveSwitchesForAutostart(&switches);
        //    // Append the old switches to the new command line.
        //    for(std::map<std::string, CommandLine::StringType>::const_iterator i=
        //        switches.begin(); i!=switches.end(); ++i)
        //    {
        //        CommandLine::StringType switch_value = i->second;
        //        if(!switch_value.empty())
        //        {
        //            new_cl->AppendSwitchNative(i->first, i->second);
        //        }
        //        else
        //        {
        //            new_cl->AppendSwitch(i->first);
        //        }
        //    }
        //    // Ensure restore last session is set.
        //    if(!new_cl->HasSwitch(switches::kRestoreLastSession))
        //    {
        //        new_cl->AppendSwitch(switches::kRestoreLastSession);
        //    }

        //    upgrade_util::RelaunchChromeBrowser(*new_cl.get());
        //}

        //if(shutdown_type_>NOT_VALID && shutdown_num_processes_>0)
        //{
        //    // Measure total shutdown time as late in the process as possible
        //    // and then write it to a file to be read at startup.
        //    // We can't use prefs since all services are shutdown at this point.
        //    TimeDelta shutdown_delta = Time::Now() - shutdown_started_;
        //    std::string shutdown_ms =
        //        base::Int64ToString(shutdown_delta.InMilliseconds());
        //    int len = static_cast<int>(shutdown_ms.length()) + 1;
        //    FilePath shutdown_ms_file = GetShutdownMsPath();
        //    file_util::WriteFile(shutdown_ms_file, shutdown_ms.c_str(), len);
        //}

        //ChromeURLDataManager::DeleteDataSources();
    }

    void SetTryingToQuit(bool quitting)
    {
        g_trying_to_quit = quitting;
    }

    bool IsTryingToQuit()
    {
        return g_trying_to_quit;
    }

    bool ShuttingDownWithoutClosingBrowsers()
    {
        return false;
    }

} //namespace browser_shutdown