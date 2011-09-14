
#include "browser_list.h"

#include "base/logging.h"
#include "base/message_loop.h"
#include "base/metric/histogram.h"

#include "browser_shutdown.h"
#include "browser_window.h"
#include "result_codes.h"
#include "tab_contents_wrapper.h"

namespace
{

    // This object is instantiated when the first Browser object is added to the
    // list and delete when the last one is removed. It watches for loads and
    // creates histograms of some global object counts.
    //class BrowserActivityObserver : public NotificationObserver
    //{
    //public:
    //    BrowserActivityObserver()
    //    {
    //        registrar_.Add(this, content::NOTIFICATION_NAV_ENTRY_COMMITTED,
    //            NotificationService::AllSources());
    //    }
    //    ~BrowserActivityObserver() {}

    //private:
    //    // NotificationObserver implementation.
    //    virtual void Observe(int type,
    //        const NotificationSource& source,
    //        const NotificationDetails& details)
    //    {
    //        DCHECK(type == content::NOTIFICATION_NAV_ENTRY_COMMITTED);
    //        const content::LoadCommittedDetails& load =
    //            *Details<content::LoadCommittedDetails>(details).ptr();
    //        if(!load.is_navigation_to_different_page())
    //        {
    //            return; // Don't log for subframes or other trivial types.
    //        }

    //        LogRenderProcessHostCount();
    //        LogBrowserTabCount();
    //    }

    //    // Counts the number of active RenderProcessHosts and logs them.
    //    void LogRenderProcessHostCount() const
    //    {
    //        int hosts_count = 0;
    //        for(RenderProcessHost::iterator i(RenderProcessHost::AllHostsIterator());
    //            !i.IsAtEnd(); i.Advance())
    //        {
    //            ++hosts_count;
    //        }
    //        UMA_HISTOGRAM_CUSTOM_COUNTS("MPArch.RPHCountPerLoad", hosts_count,
    //            1, 50, 50);
    //    }

    //    // Counts the number of tabs in each browser window and logs them. This is
    //    // different than the number of TabContents objects since TabContents objects
    //    // can be used for popups and in dialog boxes. We're just counting toplevel
    //    // tabs here.
    //    void LogBrowserTabCount() const
    //    {
    //        int tab_count = 0;
    //        for(BrowserList::const_iterator browser_iterator=BrowserList::begin();
    //            browser_iterator!=BrowserList::end(); browser_iterator++)
    //        {
    //            // Record how many tabs each window has open.
    //            UMA_HISTOGRAM_CUSTOM_COUNTS("Tabs.TabCountPerWindow",
    //                (*browser_iterator)->tab_count(), 1, 200, 50);
    //            tab_count += (*browser_iterator)->tab_count();
    //        }
    //        // Record how many tabs total are open (across all windows).
    //        UMA_HISTOGRAM_CUSTOM_COUNTS("Tabs.TabCountPerLoad", tab_count, 1, 200, 50);

    //        Browser* browser = BrowserList::GetLastActive();
    //        if(browser)
    //        {
    //            // Record how many tabs the active window has open.
    //            UMA_HISTOGRAM_CUSTOM_COUNTS("Tabs.TabCountActiveWindow",
    //                browser->tab_count(), 1, 200, 50);
    //        }
    //    }

    //    NotificationRegistrar registrar_;

    //    DISALLOW_COPY_AND_ASSIGN(BrowserActivityObserver);
    //};

    //BrowserActivityObserver* activity_observer = NULL;

    // Type used to indicate to match anything.
    const int kMatchAny                     = 0;

    // See BrowserMatches for details.
    const int kMatchOriginalProfile         = 1 << 0;
    const int kMatchCanSupportWindowFeature = 1 << 1;
    const int kMatchTabbed                  = 1 << 2;

    // Returns true if the specified |browser| matches the specified arguments.
    // |match_types| is a bitmask dictating what parameters to match:
    // . If it contains kMatchOriginalProfile then the original profile of the
    //   browser must match |profile->GetOriginalProfile()|. This is used to match
    //   incognito windows.
    // . If it contains kMatchCanSupportWindowFeature
    //   |CanSupportWindowFeature(window_feature)| must return true.
    // . If it contains kMatchTabbed, the browser must be a tabbed browser.
    bool BrowserMatches(Browser* browser,
        Profile* profile,
        Browser::WindowFeature window_feature,
        uint32 match_types)
    {
        if(match_types & kMatchCanSupportWindowFeature &&
            !browser->CanSupportWindowFeature(window_feature))
        {
            return false;
        }

        //if(match_types & kMatchOriginalProfile)
        //{
        //    if(browser->profile()->GetOriginalProfile() !=
        //        profile->GetOriginalProfile())
        //    {
        //        return false;
        //    }
        //}
        //else if(browser->profile() != profile)
        //{
        //    return false;
        //}

        if(match_types & kMatchTabbed)
        {
            return browser->is_type_tabbed();
        }

        return true;
    }

    // Returns the first browser in the specified iterator that returns true from
    // |BrowserMatches|, or null if no browsers match the arguments. See
    // |BrowserMatches| for details on the arguments.
    template<class T>
    Browser* FindBrowserMatching(const T& begin,
        const T& end,
        Profile* profile,
        Browser::WindowFeature window_feature,
        uint32 match_types)
    {
        for(T i=begin; i!=end; ++i)
        {
            if(BrowserMatches(*i, profile, window_feature, match_types))
            {
                return *i;
            }
        }
        return NULL;
    }

    Browser* FindBrowserWithTabbedOrAnyType(Profile* profile,
        bool match_tabbed,
        bool match_incognito)
    {
        uint32 match_types = kMatchAny;
        if(match_tabbed)
        {
            match_types |= kMatchTabbed;
        }
        if(match_incognito)
        {
            match_types |= kMatchOriginalProfile;
        }
        Browser* browser = FindBrowserMatching(
            BrowserList::begin_last_active(), BrowserList::end_last_active(),
            profile, Browser::FEATURE_NONE, match_types);
        // Fall back to a forward scan of all Browsers if no active one was found.
        return browser ? browser :
            FindBrowserMatching(BrowserList::begin(), BrowserList::end(), profile,
            Browser::FEATURE_NONE, match_types);
    }

    //printing::BackgroundPrintingManager* GetBackgroundPrintingManager()
    //{
    //    return g_browser_process->background_printing_manager();
    //}

    // Returns true if all browsers can be closed without user interaction.
    // This currently checks if there is pending download, or if it needs to
    // handle unload handler.
    bool AreAllBrowsersCloseable()
    {
        for(BrowserList::const_iterator i=BrowserList::begin();
            i!=BrowserList::end(); ++i)
        {
            bool normal_downloads_are_present = false;
            bool incognito_downloads_are_present = false;
            //(*i)->CheckDownloadsInProgress(&normal_downloads_are_present,
            //    &incognito_downloads_are_present);
            //if(normal_downloads_are_present ||
            //    incognito_downloads_are_present ||
            //    (*i)->TabsNeedBeforeUnloadFired())
            //{
            //    return false;
            //}
        }
        return true;
    }

}

BrowserList::BrowserVector BrowserList::browsers_;
ObserverList<BrowserList::Observer> BrowserList::observers_;

// static
void BrowserList::AddBrowser(Browser* browser)
{
    DCHECK(browser);
    browsers_.push_back(browser);

    //if(!activity_observer)
    //{
    //    activity_observer = new BrowserActivityObserver;
    //}

    //NotificationService::current()->Notify(
    //    chrome::NOTIFICATION_BROWSER_OPENED,
    //    Source<Browser>(browser),
    //    NotificationService::NoDetails());

    // Send out notifications after add has occurred. Do some basic checking to
    // try to catch evil observers that change the list from under us.
    size_t original_count = observers_.size();
    FOR_EACH_OBSERVER(Observer, observers_, OnBrowserAdded(browser));
    DCHECK_EQ(original_count, observers_.size())
        << "observer list modified during notification";
}

// static
void BrowserList::MarkAsCleanShutdown()
{
    for(const_iterator i=begin(); i!=end(); ++i)
    {
        //(*i)->profile()->MarkAsCleanShutdown();
    }
}

void BrowserList::AttemptExitInternal()
{
    //NotificationService::current()->Notify(
    //    content::NOTIFICATION_APP_EXITING,
    //    NotificationService::AllSources(),
    //    NotificationService::NoDetails());

    // On most platforms, closing all windows causes the application to exit.
    CloseAllBrowsers();
}

// static
void BrowserList::NotifyAndTerminate(bool fast_path)
{
    if(fast_path)
    {
        //NotificationService::current()->Notify(
        //    content::NOTIFICATION_APP_TERMINATING,
        //    NotificationService::AllSources(),
        //    NotificationService::NoDetails());
    }

    AllBrowsersClosedAndAppExiting();
}

// static
void BrowserList::AllBrowsersClosedAndAppExiting()
{
    view::Widget::CloseAllSecondaryWidgets();
}

// static
void BrowserList::RemoveBrowser(Browser* browser)
{
    RemoveBrowserFrom(browser, &last_active_browsers_);

    // Closing all windows does not indicate quitting the application on the Mac,
    // however, many UI tests rely on this behavior so leave it be for now and
    // simply ignore the behavior on the Mac outside of unit tests.
    // TODO(andybons): Fix the UI tests to Do The Right Thing.
    bool closing_last_browser = (browsers_.size() == 1);
    //NotificationService::current()->Notify(
    //    chrome::NOTIFICATION_BROWSER_CLOSED,
    //    Source<Browser>(browser), Details<bool>(&closing_last_browser));

    RemoveBrowserFrom(browser, &browsers_);

    // Do some basic checking to try to catch evil observers
    // that change the list from under us.
    size_t original_count = observers_.size();
    FOR_EACH_OBSERVER(Observer, observers_, OnBrowserRemoved(browser));
    DCHECK_EQ(original_count, observers_.size())
        << "observer list modified during notification";

    // If the last Browser object was destroyed, make sure we try to close any
    // remaining dependent windows too.
    if(browsers_.empty())
    {
        //delete activity_observer;
        //activity_observer = NULL;
    }

    // If we're exiting, send out the APP_TERMINATING notification to allow other
    // modules to shut themselves down.
    if(browsers_.empty() && browser_shutdown::IsTryingToQuit())
    {
        // Last browser has just closed, and this is a user-initiated quit or there
        // is no module keeping the app alive, so send out our notification. No need
        // to call ProfileManager::ShutdownSessionServices() as part of the
        // shutdown, because Browser::WindowClosing() already makes sure that the
        // SessionService is created and notified.
        //NotificationService::current()->Notify(
        //    content::NOTIFICATION_APP_TERMINATING,
        //    NotificationService::AllSources(),
        //    NotificationService::NoDetails());
        AllBrowsersClosedAndAppExiting();
    }
}

// static
void BrowserList::AddObserver(BrowserList::Observer* observer)
{
    observers_.AddObserver(observer);
}

// static
void BrowserList::RemoveObserver(BrowserList::Observer* observer)
{
    observers_.RemoveObserver(observer);
}

// static
void BrowserList::CloseAllBrowsers()
{
    bool session_ending =
        browser_shutdown::GetShutdownType() == browser_shutdown::END_SESSION;
    bool force_exit = false;
    // Tell everyone that we are shutting down.
    browser_shutdown::SetTryingToQuit(true);

    // Before we close the browsers shutdown all session services. That way an
    // exit can restore all browsers open before exiting.
    //ProfileManager::ShutdownSessionServices();

    // If there are no browsers, send the APP_TERMINATING action here. Otherwise,
    // it will be sent by RemoveBrowser() when the last browser has closed.
    if(force_exit || browsers_.empty())
    {
        NotifyAndTerminate(true);
        return;
    }
    for(BrowserList::const_iterator i=BrowserList::begin();
        i!=BrowserList::end();)
    {
        Browser* browser = *i;
        browser->window()->Close();
        if(!session_ending)
        {
            ++i;
        }
        else
        {
            // This path is hit during logoff/power-down. In this case we won't get
            // a final message and so we force the browser to be deleted.
            // Close doesn't immediately destroy the browser
            // (Browser::TabStripEmpty() uses invoke later) but when we're ending the
            // session we need to make sure the browser is destroyed now. So, invoke
            // DestroyBrowser to make sure the browser is deleted and cleanup can
            // happen.
            while(browser->tab_count())
            {
                delete browser->GetTabContentsWrapperAt(0);
            }
            browser->window()->DestroyBrowser();
            i = BrowserList::begin();
            if(i!=BrowserList::end() && browser==*i)
            {
                // Destroying the browser should have removed it from the browser list.
                // We should never get here.
                NOTREACHED();
                return;
            }
        }
    }
}

void BrowserList::CloseAllBrowsersWithProfile(Profile* profile)
{
    BrowserVector browsers_to_close;
    for(BrowserList::const_iterator i=BrowserList::begin();
        i!=BrowserList::end(); ++i)
    {
        //if((*i)->profile() == profile)
        {
            browsers_to_close.push_back(*i);
        }
    }

    for(BrowserVector::const_iterator i=browsers_to_close.begin();
        i!=browsers_to_close.end(); ++i)
    {
        (*i)->window()->Close();
    }
}

// static
void BrowserList::AttemptUserExit()
{
    // Reset the restart bit that might have been set in cancelled restart
    // request.
    //PrefService* pref_service = g_browser_process->local_state();
    //pref_service->SetBoolean(prefs::kRestartLastSessionOnShutdown, false);
    AttemptExitInternal();
}

// static
void BrowserList::AttemptRestart()
{
    // Set the flag to restore state after the restart.
    //PrefService* pref_service = g_browser_process->local_state();
    //pref_service->SetBoolean(prefs::kRestartLastSessionOnShutdown, true);
    AttemptExit();
}

// static
void BrowserList::AttemptExit()
{
    // If we know that all browsers can be closed without blocking,
    // don't notify users of crashes beyond this point.
    // Note that MarkAsCleanShutdown does not set UMA's exit cleanly bit
    // so crashes during shutdown are still reported in UMA.
    if(AreAllBrowsersCloseable())
    {
        MarkAsCleanShutdown();
    }
    AttemptExitInternal();
}

// static
void BrowserList::SessionEnding()
{
    // EndSession is invoked once per frame. Only do something the first time.
    static bool already_ended = false;
    // We may get called in the middle of shutdown, e.g. http://crbug.com/70852
    // In this case, do nothing.
    if(already_ended/* || !NotificationService::current()*/)
    {
        return;
    }
    already_ended = true;

    browser_shutdown::OnShutdownStarting(browser_shutdown::END_SESSION);

    //NotificationService::current()->Notify(
    //    content::NOTIFICATION_APP_EXITING,
    //    NotificationService::AllSources(),
    //    NotificationService::NoDetails());

    BrowserList::CloseAllBrowsers();

    // Send out notification. This is used during testing so that the test harness
    // can properly shutdown before we exit.
    //NotificationService::current()->Notify(
    //    chrome::NOTIFICATION_SESSION_END,
    //    NotificationService::AllSources(),
    //    NotificationService::NoDetails());

    // And shutdown.
    browser_shutdown::Shutdown();

    // At this point the message loop is still running yet we've shut everything
    // down. If any messages are processed we'll likely crash. Exit now.
    ExitProcess(content::RESULT_CODE_NORMAL_EXIT);
}

// static
bool BrowserList::HasBrowserWithProfile(Profile* profile)
{
    return FindBrowserMatching(BrowserList::begin(),
        BrowserList::end(),
        profile,
        Browser::FEATURE_NONE,
        kMatchAny) != NULL;
}

// static
int BrowserList::keep_alive_count_ = 0;

// static
void BrowserList::StartKeepAlive()
{
    // Increment the browser process refcount as long as we're keeping the
    // application alive.
    if(!WillKeepAlive())
    {
        //g_browser_process->AddRefModule();
    }
    keep_alive_count_++;
}

// static
void BrowserList::EndKeepAlive()
{
    DCHECK_GT(keep_alive_count_, 0);
    keep_alive_count_--;
    // Allow the app to shutdown again.
    if(!WillKeepAlive())
    {
        //g_browser_process->ReleaseModule();
        // If there are no browsers open and we aren't already shutting down,
        // initiate a shutdown. Also skips shutdown if this is a unit test
        // (MessageLoop::current() == null).
        if(browsers_.empty() && !browser_shutdown::IsTryingToQuit() &&
            MessageLoop::current())
        {
            CloseAllBrowsers();
        }
    }
}

// static
bool BrowserList::WillKeepAlive()
{
    return keep_alive_count_ > 0;
}

// static
BrowserList::BrowserVector BrowserList::last_active_browsers_;

// static
void BrowserList::SetLastActive(Browser* browser)
{
    RemoveBrowserFrom(browser, &last_active_browsers_);
    last_active_browsers_.push_back(browser);

    FOR_EACH_OBSERVER(Observer, observers_, OnBrowserSetLastActive(browser));
}

// static
Browser* BrowserList::GetLastActive()
{
    if(!last_active_browsers_.empty())
    {
        return *(last_active_browsers_.rbegin());
    }

    return NULL;
}

// static
Browser* BrowserList::GetLastActiveWithProfile(Profile* profile)
{
    // We are only interested in last active browsers, so we don't fall back to
    // all browsers like FindBrowserWith* do.
    return FindBrowserMatching(
        BrowserList::begin_last_active(), BrowserList::end_last_active(), profile,
        Browser::FEATURE_NONE, kMatchAny);
}

// static
Browser* BrowserList::FindTabbedBrowser(Profile* profile,
                                        bool match_incognito)
{
    return FindBrowserWithTabbedOrAnyType(profile, true, match_incognito);
}

// static
Browser* BrowserList::FindAnyBrowser(Profile* profile, bool match_incognito)
{
    return FindBrowserWithTabbedOrAnyType(profile, false, match_incognito);
}

// static
Browser* BrowserList::FindBrowserWithFeature(Profile* profile,
                                             Browser::WindowFeature feature)
{
    Browser* browser = FindBrowserMatching(
        BrowserList::begin_last_active(), BrowserList::end_last_active(),
        profile, feature, kMatchCanSupportWindowFeature);
    // Fall back to a forward scan of all Browsers if no active one was found.
    return browser ? browser :
        FindBrowserMatching(BrowserList::begin(), BrowserList::end(), profile,
        feature, kMatchCanSupportWindowFeature);
}

// static
Browser* BrowserList::FindBrowserWithProfile(Profile* profile)
{
    return FindAnyBrowser(profile, false);
}

// static
Browser* BrowserList::FindBrowserWithID(SessionID::id_type desired_id)
{
    for(BrowserList::const_iterator i=BrowserList::begin();
        i!=BrowserList::end(); ++i)
    {
        if((*i)->session_id().id() == desired_id)
        {
            return *i;
        }
    }
    return NULL;
}

// static
Browser* BrowserList::FindBrowserWithWindow(HWND window)
{
    for(BrowserList::const_iterator it=BrowserList::begin();
        it!=BrowserList::end(); ++it)
    {
        Browser* browser = *it;
        if(browser->window() && browser->window()->GetNativeHandle()==window)
        {
            return browser;
        }
    }
    return NULL;
}

// static
size_t BrowserList::GetBrowserCountForType(Profile* profile,
                                           bool match_tabbed)
{
    size_t result = 0;
    for(BrowserList::const_iterator i=BrowserList::begin();
        i!=BrowserList::end(); ++i)
    {
        if(BrowserMatches(*i, profile, Browser::FEATURE_NONE,
            match_tabbed ? kMatchTabbed : kMatchAny))
        {
            ++result;
        }
    }
    return result;
}

// static
size_t BrowserList::GetBrowserCount(Profile* profile)
{
    size_t result = 0;
    for(BrowserList::const_iterator i=BrowserList::begin();
        i!=BrowserList::end(); ++i)
    {
        if(BrowserMatches(*i, profile, Browser::FEATURE_NONE, kMatchAny))
        {
            result++;
        }
    }
    return result;
}

// static
void BrowserList::RemoveBrowserFrom(Browser* browser,
                                    BrowserVector* browser_list)
{
    const iterator remove_browser =
        std::find(browser_list->begin(), browser_list->end(), browser);
    if(remove_browser != browser_list->end())
    {
        browser_list->erase(remove_browser);
    }
}

TabContentsIterator::TabContentsIterator()
: browser_iterator_(BrowserList::begin()),
web_view_index_(-1),
//bg_printing_iterator_(GetBackgroundPrintingManager()->begin()),
cur_(NULL)
{
    Advance();
}

void TabContentsIterator::Advance()
{
    // The current TabContents should be valid unless we are at the beginning.
    DCHECK(cur_ || (web_view_index_ == -1 &&
        browser_iterator_ == BrowserList::begin()))
        << "Trying to advance past the end";

    // Update cur_ to the next TabContents in the list.
    while(browser_iterator_ != BrowserList::end())
    {
        if(++web_view_index_ >= (*browser_iterator_)->tab_count())
        {
            // Advance to the next Browser in the list.
            ++browser_iterator_;
            web_view_index_ = -1;
            continue;
        }

        TabContentsWrapper* next_tab =
            (*browser_iterator_)->GetTabContentsWrapperAt(web_view_index_);
        if(next_tab)
        {
            cur_ = next_tab;
            return;
        }
    }
    // If no more TabContents from Browsers, check the BackgroundPrintingManager.
    //while(bg_printing_iterator_ != GetBackgroundPrintingManager()->end())
    //{
    //    cur_ = *bg_printing_iterator_;
    //    CHECK(cur_);
    //    ++bg_printing_iterator_;
    //    return;
    //}
    // Reached the end - no more TabContents.
    cur_ = NULL;
}