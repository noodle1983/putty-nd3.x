
#include "browser.h"

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <string>

#include "base/message_loop.h"
#include "base/threading/thread_restrictions.h"
#include "base/time.h"

#include "SkBitmap.h"

#include "ui_base/l10n/l10n_util.h"

#include "../wanui_res/resource.h"

#include "browser_list.h"
#include "browser_navigator.h"
#include "browser_shutdown.h"
#include "browser_window.h"
#include "location_bar.h"
#include "tab_contents.h"
#include "tab_contents_view.h"
#include "tab_contents_wrapper.h"
#include "tab_strip_model.h"
#include "tab_strip_model_delegate.h"
#include "view/widget/monitor_win.h"
#include "chrome_command_ids.h"

using base::TimeDelta;

///////////////////////////////////////////////////////////////////////////////

namespace
{

    // The URL to be loaded to display Help.
    const char kHelpContentUrl[] =
        "https://www.google.com/support/chrome/";

    // The URL to be opened when the Help link on the Autofill dialog is clicked.
    const char kAutofillHelpUrl[] =
        "https://www.google.com/support/chrome/bin/answer.py?answer=142893";

    // The URL to be loaded to display the "Report a broken page" form.
    const char kBrokenPageUrl[] =
        "https://www.google.com/support/chrome/bin/request.py?contact_type="
        "broken_website&format=inproduct&p.page_title=$1&p.page_url=$2";

    // The URL for the privacy dashboard.
    const char kPrivacyDashboardUrl[] = "https://www.google.com/dashboard";

    // How long we wait before updating the browser chrome while loading a page.
    const int kUIUpdateCoalescingTimeMS = 200;

    const char kHashMark[] = "#";
}

////////////////////////////////////////////////////////////////////////////////
// Browser, CreateParams:

Browser::CreateParams::CreateParams(Type type)
: type(type) {}

///////////////////////////////////////////////////////////////////////////////
// Browser, Constructors, Creation, Showing:

Browser::Browser(Type type)
: type_(type),
window_(NULL),
tab_handler_(TabHandler::CreateTabHandler(this)),
toolbar_model_(this),
chrome_updater_factory_(this),
is_attempting_to_close_browser_(false),
show_state_(ui::SHOW_STATE_DEFAULT),
is_session_restore_(false),
method_factory_(this),
block_command_execution_(false),
last_blocked_command_id_(-1),
last_blocked_command_disposition_(CURRENT_TAB),
bookmark_bar_state_(BookmarkBar::HIDDEN),
fullscreened_tab_(NULL),
tab_caused_fullscreen_(false)
{
    InitCommandState();
    BrowserList::AddBrowser(this);

    UpdateTabStripModelInsertionPolicy();

    UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_INIT);
}

Browser::~Browser()
{
    BrowserList::RemoveBrowser(this);
}

// static
Browser* Browser::Create()
{
    Browser* browser = new Browser(TYPE_TABBED);
	const int width = 800;
	const int height = 600;
	gfx::Rect monitorRect = view::GetMonitorBoundsForRect(gfx::Rect(0, 0, 10, 10));
	browser->set_override_bounds(gfx::Rect((monitorRect.width() - width)/2, (monitorRect.height() - height)/2, width, height));
    browser->InitBrowserWindow();
    return browser;
}

// static
Browser* Browser::CreateWithParams(const CreateParams& params)
{
    Browser* browser = new Browser(params.type);
    if(!params.initial_bounds.IsEmpty())
    {
        browser->set_override_bounds(params.initial_bounds);
    }
    browser->InitBrowserWindow();
    return browser;
}

// static
Browser* Browser::CreateForType(Type type)
{
    CreateParams params(type);
    return CreateWithParams(params);
}

void Browser::InitBrowserWindow()
{
    DCHECK(!window_);

    window_ = CreateBrowserWindow();

    {
        // TODO: This might hit the disk
        //   http://code.google.com/p/chromium/issues/detail?id=61638
        base::ThreadRestrictions::ScopedAllowIO allow_io;

        // Set the app user model id for this application to that of the application
        // name.  See http://crbug.com/7028.
        //ui::win::SetAppIdForWindow(
        //    ShellIntegration::GetChromiumAppId(profile_->GetPath()),
        //    window()->GetNativeHandle());
    }
}

///////////////////////////////////////////////////////////////////////////////
// Browser, Creation Helpers:

// static
void Browser::OpenEmptyWindow()
{
    Browser* browser = Browser::Create();
    browser->AddBlankTab(true);
    browser->window()->Show();
}

// static
void Browser::OpenBookmarkManagerWindow()
{
    Browser* browser = Browser::Create();
    browser->OpenBookmarkManager();
    browser->window()->Show();
}

///////////////////////////////////////////////////////////////////////////////
// Browser, State Storage and Retrieval for UI:

std::string Browser::GetWindowPlacementKey() const
{
    std::string name("browser.window_placement");
    return name;
}

bool Browser::ShouldSaveWindowPlacement() const
{
    switch(type_)
    {
    case TYPE_TABBED:
        return true;
    case TYPE_PANEL:
        // Do not save the window placement of panels.
        return false;
    default:
        return false;
    }
}

void Browser::SaveWindowPlacement(const gfx::Rect& bounds,
                                  ui::WindowShowState show_state)
{
    // Save to the session storage service, used when reloading a past session.
    // Note that we don't want to be the ones who cause lazy initialization of
    // the session service. This function gets called during initial window
    // showing, and we don't want to bring in the session service this early.
    //SessionService* session_service =
    //    SessionServiceFactory::GetForProfileIfExisting(profile());
    //if(session_service)
    //{
    //    session_service->SetWindowBounds(session_id_, bounds, show_state);
    //}
}

gfx::Rect Browser::GetSavedWindowBounds() const
{
    gfx::Rect restored_bounds = override_bounds_;
    //bool maximized;
    //WindowSizer::GetBrowserWindowBounds(app_name_, restored_bounds, this,
    //    &restored_bounds, &maximized);
    return restored_bounds;
}

// TODO(beng): obtain maximized state some other way so we don't need to go
//             through all this hassle.
ui::WindowShowState Browser::GetSavedWindowShowState() const
{
    if(show_state_ != ui::SHOW_STATE_DEFAULT)
    {
        return show_state_;
    }

    // An explicit maximized state was not set. Query the window sizer.
    gfx::Rect restored_bounds;
    bool maximized = false;
    //WindowSizer::GetBrowserWindowBounds(app_name_, restored_bounds, this,
    //    &restored_bounds, &maximized);
    return maximized ? ui::SHOW_STATE_MAXIMIZED : ui::SHOW_STATE_NORMAL;
}

SkBitmap Browser::GetCurrentPageIcon() const
{
    return SkBitmap();
    //TabContentsWrapper* contents = GetSelectedTabContentsWrapper();
    //// |contents| can be NULL since GetCurrentPageIcon() is called by the window
    //// during the window's creation (before tabs have been added).
    //return contents ? contents->favicon_tab_helper()->GetFavicon() : SkBitmap();
}

string16 Browser::GetWindowTitleForCurrentTab() const
{
    TabContents* contents = GetSelectedTabContents();
    string16 title;

    // |contents| can be NULL because GetWindowTitleForCurrentTab is called by the
    // window during the window's creation (before tabs have been added).
    if(contents)
    {
        title = contents->GetTitle();
        FormatTitleForDisplay(&title);
    }
    if(title.empty())
    {
        title = TabContentsWrapper::GetDefaultTitle();
    }

    int string_id = IDS_BROWSER_WINDOW_TITLE_FORMAT;
    return ui::GetStringFUTF16(string_id, title);
}

// static
void Browser::FormatTitleForDisplay(string16* title)
{
    size_t current_index = 0;
    size_t match_index;
    while((match_index = title->find(L'\n', current_index)) != string16::npos)
    {
        title->replace(match_index, 1, string16());
        current_index = match_index;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Browser, OnBeforeUnload handling:

bool Browser::TabsNeedBeforeUnloadFired()
{
    if(tabs_needing_before_unload_fired_.empty())
    {
        for(int i=0; i<tab_count(); ++i)
        {
            TabContents* contents = GetTabContentsAt(i);
            if(contents->NeedToFireBeforeUnload())
            {
                tabs_needing_before_unload_fired_.insert(contents);
            }
        }
    }
    return !tabs_needing_before_unload_fired_.empty();
}

bool Browser::ShouldCloseWindow()
{
    if(HasCompletedUnloadProcessing())
    {
        return IsClosingPermitted();
    }

    is_attempting_to_close_browser_ = true;

    if(!TabsNeedBeforeUnloadFired())
    {
        return IsClosingPermitted();
    }

    ProcessPendingTabs();
    return false;
}

void Browser::OnWindowClosing()
{
    if(!ShouldCloseWindow())
    {
        return;
    }

    bool exiting = false;

    // Application should shutdown on last window close if the user is explicitly
    // trying to quit, or if there is nothing keeping the browser alive (such as
    // AppController on the Mac, or BackgroundContentsService for background
    // pages).
    bool should_quit_if_last_browser =
        browser_shutdown::IsTryingToQuit() || !BrowserList::WillKeepAlive();

    if(should_quit_if_last_browser && BrowserList::size()==1)
    {
        browser_shutdown::OnShutdownStarting(browser_shutdown::WINDOW_CLOSE);
        exiting = true;
    }

    CloseAllTabs();
}

void Browser::OnWindowActivated()
{
    // On some platforms we want to automatically reload tabs that are
    // killed when the user selects them.
    TabContents* contents = GetSelectedTabContents();
    if(contents && contents->crashed_status() ==
        base::TERMINATION_STATUS_PROCESS_WAS_KILLED)
    {
        //if (CommandLine::ForCurrentProcess()->HasSwitch(
        //    switches::kReloadKilledTabs))
        //{
        //    Reload(CURRENT_TAB);
        //}
    }
}

////////////////////////////////////////////////////////////////////////////////
// Browser, TabStripModel pass-thrus:

int Browser::tab_count() const
{
    return tab_handler_->GetTabStripModel()->count();
}

int Browser::active_index() const
{
    return tab_handler_->GetTabStripModel()->active_index();
}

TabContentsWrapper* Browser::GetSelectedTabContentsWrapper() const
{
    return tabstrip_model()->GetActiveTabContents();
}

TabContentsWrapper* Browser::GetTabContentsWrapperAt(int index) const
{
    return tabstrip_model()->GetTabContentsAt(index);
}

TabContents* Browser::GetSelectedTabContents() const
{
    TabContentsWrapper* wrapper = GetSelectedTabContentsWrapper();
    if(wrapper)
    {
        return wrapper->tab_contents();
    }
    return NULL;
}

TabContents* Browser::GetTabContentsAt(int index) const
{
    TabContentsWrapper* wrapper = tabstrip_model()->GetTabContentsAt(index);
    if(wrapper)
    {
        return wrapper->tab_contents();
    }
    return NULL;
}

void Browser::ActivateTabAt(int index, bool user_gesture)
{
    tab_handler_->GetTabStripModel()->ActivateTabAt(index, user_gesture);
}

void Browser::CloseAllTabs()
{
    tab_handler_->GetTabStripModel()->CloseAllTabs();
}

////////////////////////////////////////////////////////////////////////////////
// Browser, Tab adding/showing functions:

int Browser::GetIndexForInsertionDuringRestore(int relative_index)
{
    return (tab_handler_->GetTabStripModel()->insertion_policy() ==
        TabStripModel::INSERT_AFTER) ? tab_count() : relative_index;
}

TabContentsWrapper* Browser::AddSelectedTabWithURL(const Url& url)
{
    browser::NavigateParams params(this, url);
    params.disposition = NEW_FOREGROUND_TAB;
    browser::Navigate(&params);
    return params.target_contents;
}

TabContents* Browser::AddTab(TabContentsWrapper* tab_contents)
{
    tab_handler_->GetTabStripModel()->AddTabContents(
        tab_contents, -1, TabStripModel::ADD_ACTIVE);
    return tab_contents->tab_contents();
}

void Browser::AddTabContents(TabContents* new_contents,
                             WindowOpenDisposition disposition,
                             const gfx::Rect& initial_pos,
                             bool user_gesture)
{
    AddNewContents(NULL, new_contents, disposition, initial_pos, user_gesture);
}

void Browser::CloseTabContents(TabContents* contents)
{
    CloseContents(contents);
}

void Browser::BrowserRenderWidgetShowing()
{
    RenderWidgetShowing();
}

void Browser::BookmarkBarSizeChanged(bool is_animating)
{
    window_->ToolbarSizeChanged(is_animating);
}

bool Browser::NavigateToIndexWithDisposition(int index,
                                             WindowOpenDisposition disp)
{
    //NavigationController& controller =
    //    GetOrCloneTabForDisposition(disp)->controller();
    //if(index < 0 || index >= controller.entry_count())
    //{
    //    return false;
    //}
    //controller.GoToIndex(index);
    return true;
}

void Browser::ShowSingletonTab(const Url& url)
{
    //browser::NavigateParams params(GetSingletonTabNavigateParams(url));
    //browser::Navigate(&params);
}

void Browser::ShowSingletonTabRespectRef(const Url& url)
{
    //browser::NavigateParams params(GetSingletonTabNavigateParams(url));
    //params.ref_behavior = browser::NavigateParams::RESPECT_REF;
    //browser::Navigate(&params);
}

void Browser::WindowFullscreenStateChanged()
{
    UpdateCommandsForFullscreenMode(window_->IsFullscreen());
    UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_TOGGLE_FULLSCREEN);
    MessageLoop::current()->PostTask(method_factory_.NewRunnableMethod(
        &Browser::NotifyFullscreenChange));
}

void Browser::NotifyFullscreenChange()
{
    //NotificationService::current()->Notify(
    //    chrome::NOTIFICATION_FULLSCREEN_CHANGED,
    //    Source<Browser>(this),
    //    NotificationService::NoDetails());
}

///////////////////////////////////////////////////////////////////////////////
// Browser, Assorted browser commands:

TabContents* Browser::GetOrCloneTabForDisposition(
    WindowOpenDisposition disposition)
{
    TabContentsWrapper* current_tab = GetSelectedTabContentsWrapper();
    switch(disposition)
    {
    case NEW_FOREGROUND_TAB:
    case NEW_BACKGROUND_TAB:
        {
            current_tab = current_tab->Clone();
            tab_handler_->GetTabStripModel()->AddTabContents(
                current_tab, -1,
                disposition==NEW_FOREGROUND_TAB ? TabStripModel::ADD_ACTIVE :
                TabStripModel::ADD_NONE);
            break;
        }
    case NEW_WINDOW:
        {
            current_tab = current_tab->Clone();
            Browser* browser = Browser::Create();
            browser->tabstrip_model()->AddTabContents(
                current_tab, -1, TabStripModel::ADD_ACTIVE);
            browser->window()->Show();
            break;
        }
    default:
        break;
    }
    return current_tab->tab_contents();
}

void Browser::UpdateTabStripModelInsertionPolicy()
{
    tab_handler_->GetTabStripModel()->SetInsertionPolicy(
        TabStripModel::INSERT_AFTER);
}

bool Browser::SupportsWindowFeatureImpl(WindowFeature feature,
                                        bool check_fullscreen) const
{
    // On Mac, fullscreen mode has most normal things (in a slide-down panel). On
    // other platforms, we hide some controls when in fullscreen mode.
    bool hide_ui_for_fullscreen = false;
    hide_ui_for_fullscreen = check_fullscreen && window_ &&
        window_->IsFullscreen();

    unsigned int features = FEATURE_INFOBAR | FEATURE_SIDEBAR;

    if(is_type_tabbed())
    {
        features |= FEATURE_BOOKMARKBAR;
    }

    if(!hide_ui_for_fullscreen)
    {
        if(!is_type_tabbed())
        {
            features |= FEATURE_TITLEBAR;
        }

        if(is_type_tabbed())
        {
            features |= FEATURE_TABSTRIP;
        }

        if(is_type_tabbed())
        {
            features |= FEATURE_TOOLBAR;
        }

        features |= FEATURE_LOCATIONBAR;
    }
    return !!(features & feature);
}

bool Browser::IsClosingPermitted()
{
    return true;
    //TabCloseableStateWatcher* watcher =
    //    g_browser_process->tab_closeable_state_watcher();
    //bool can_close = !watcher || watcher->CanCloseBrowser(this);
    //if(!can_close && is_attempting_to_close_browser_)
    //{
    //    CancelWindowClose();
    //}
    //return can_close;
}

bool Browser::CanGoBack() const
{
    return false;
    //return GetSelectedTabContentsWrapper()->controller().CanGoBack();
}

void Browser::GoBack(WindowOpenDisposition disposition)
{
    //TabContentsWrapper* current_tab = GetSelectedTabContentsWrapper();
    //if(CanGoBack())
    //{
    //    TabContents* new_tab = GetOrCloneTabForDisposition(disposition);
    //    // If we are on an interstitial page and clone the tab, it won't be copied
    //    // to the new tab, so we don't need to go back.
    //    if(current_tab->tab_contents()->showing_interstitial_page() &&
    //        (new_tab != current_tab->tab_contents()))
    //    {
    //        return;
    //    }
    //    new_tab->controller().GoBack();
    //}
}

bool Browser::CanGoForward() const
{
    return false;
    //return GetSelectedTabContentsWrapper()->controller().CanGoForward();
}

void Browser::GoForward(WindowOpenDisposition disposition)
{
    //if(CanGoForward())
    //{
    //    GetOrCloneTabForDisposition(disposition)->controller().GoForward();
    //}
}

void Browser::Reload(WindowOpenDisposition disposition)
{
    ReloadInternal(disposition, false);
}

void Browser::ReloadIgnoringCache(WindowOpenDisposition disposition)
{
    ReloadInternal(disposition, true);
}

void Browser::ReloadInternal(WindowOpenDisposition disposition,
                             bool ignore_cache)
{
    // If we are showing an interstitial, treat this as an OpenURL.
    //TabContents* current_tab = GetSelectedTabContents();
    //if(current_tab && current_tab->showing_interstitial_page())
    //{
    //    NavigationEntry* entry = current_tab->controller().GetActiveEntry();
    //    DCHECK(entry);  // Should exist if interstitial is showing.
    //    OpenURL(entry->url(), GURL(), disposition, PageTransition::RELOAD);
    //    return;
    //}

    //// As this is caused by a user action, give the focus to the page.
    //TabContents* tab = GetOrCloneTabForDisposition(disposition);
    //if(!tab->FocusLocationBarByDefault())
    //{
    //    tab->Focus();
    //}
    //if(ignore_cache)
    //{
    //    tab->controller().ReloadIgnoringCache(true);
    //}
    //else
    //{
    //    tab->controller().Reload(true);
    //}
}

void Browser::Home(WindowOpenDisposition disposition)
{
    OpenURL(GetHomePage(), Url(), disposition);
}

void Browser::OpenCurrentURL()
{
    LocationBar* location_bar = window_->GetLocationBar();
    if(!location_bar)
    {
        return;
    }

    WindowOpenDisposition open_disposition =
        location_bar->GetWindowOpenDisposition();
    //if(OpenInstant(open_disposition))
    //{
    //    return;
    //}

    //Url url(location_bar->GetInputString());

    //if(open_disposition==CURRENT_TAB && TabFinder::IsEnabled())
    //{
    //    Browser* existing_browser = NULL;
    //    TabContents* existing_tab = TabFinder::GetInstance()->FindTab(
    //        this, url, &existing_browser);
    //    if(existing_tab)
    //    {
    //        existing_browser->ActivateContents(existing_tab);
    //        return;
    //    }
    //}

    browser::NavigateParams params(this, Url()/*url*/);
    params.disposition = open_disposition;
    // Use ADD_INHERIT_OPENER so that all pages opened by the omnibox at least
    // inherit the opener. In some cases the tabstrip will determine the group
    // should be inherited, in which case the group is inherited instead of the
    // opener.
    params.tabstrip_add_types =
        TabStripModel::ADD_FORCE_INDEX | TabStripModel::ADD_INHERIT_OPENER;
    browser::Navigate(&params);
}

void Browser::Stop()
{
    GetSelectedTabContentsWrapper()->tab_contents()->Stop();
}

void Browser::NewWindow()
{
    //IncognitoModePrefs::Availability incognito_avail =
    //    IncognitoModePrefs::GetAvailability(profile_->GetPrefs());
    //if(browser_defaults::kAlwaysOpenIncognitoWindow &&
    //    incognito_avail != IncognitoModePrefs::DISABLED &&
    //    (CommandLine::ForCurrentProcess()->HasSwitch(switches::kIncognito) ||
    //    incognito_avail == IncognitoModePrefs::FORCED))
    //{
    //    NewIncognitoWindow();
    //    return;
    //}
    //SessionService* session_service =
    //    SessionServiceFactory::GetForProfile(profile_->GetOriginalProfile());
    //if(!session_service ||
    //    !session_service->RestoreIfNecessary(std::vector<GURL>()))
    //{
    //    Browser::OpenEmptyWindow(profile_->GetOriginalProfile());
    //}
}

void Browser::CloseWindow()
{
    window_->Close();
}

void Browser::NewTab()
{
    if(is_type_tabbed())
    {
        AddBlankTab(true);
        GetSelectedTabContentsWrapper()->tab_contents()->puttyView()->RequestFocus();
    }
    else
    {
        Browser* b = GetOrCreateTabbedBrowser();
        b->AddBlankTab(true);
        b->window()->Show();
        // The call to AddBlankTab above did not set the focus to the tab as its
        // window was not active, so we have to do it explicitly.
        // See http://crbug.com/6380.
        b->GetSelectedTabContentsWrapper()->tab_contents()->puttyView()->RequestFocus();
    
    }
}

void Browser::CloseTab()
{
    if(CanCloseTab())
    {
        tab_handler_->GetTabStripModel()->CloseSelectedTabs();
    }
}

void Browser::SelectNextTab()
{
    tab_handler_->GetTabStripModel()->SelectNextTab();
}

void Browser::SelectPreviousTab()
{
    tab_handler_->GetTabStripModel()->SelectPreviousTab();
}

void Browser::OpenTabpose()
{
    NOTREACHED();
}

void Browser::MoveTabNext()
{
    tab_handler_->GetTabStripModel()->MoveTabNext();
}

void Browser::MoveTabPrevious()
{
    tab_handler_->GetTabStripModel()->MoveTabPrevious();
}

void Browser::SelectNumberedTab(int index)
{
    if(index < tab_count())
    {
        tab_handler_->GetTabStripModel()->ActivateTabAt(index, true);
    }
}

void Browser::SelectLastTab()
{
    tab_handler_->GetTabStripModel()->SelectLastTab();
}

void Browser::DuplicateTab()
{
    DuplicateContentsAt(active_index());
}

void Browser::WriteCurrentURLToClipboard()
{
    // TODO(ericu): There isn't currently a metric for this.  Should there be?
    // We don't appear to track the action when it comes from the
    // RenderContextViewMenu.

    //TabContents* contents = GetSelectedTabContents();
    //if(!contents->ShouldDisplayURL())
    //{
    //    return;
    //}

    //chrome_browser_net::WriteURLToClipboard(
    //    contents->GetURL(),
    //    profile_->GetPrefs()->GetString(prefs::kAcceptLanguages),
    //    g_browser_process->clipboard());
}

void Browser::ConvertPopupToTabbedBrowser()
{
    int tab_strip_index = tab_handler_->GetTabStripModel()->active_index();
    TabContentsWrapper* contents =
        tab_handler_->GetTabStripModel()->DetachTabContentsAt(tab_strip_index);
    Browser* browser = Browser::Create();
    browser->tabstrip_model()->AppendTabContents(contents, true);
    browser->window()->Show();
}

void Browser::ToggleFullscreenMode()
{
    bool entering_fullscreen = !window_->IsFullscreen();

    // In kiosk mode, we always want to be fullscreen. When the browser first
    // starts we're not yet fullscreen, so let the initial toggle go through.
    //if(CommandLine::ForCurrentProcess()->HasSwitch(switches::kKioskMode) &&
    //    window_->IsFullscreen())
    //{
    //    return;
    //}

    window_->SetFullscreen(!window_->IsFullscreen());

    // Once the window has become fullscreen it'll call back to
    // WindowFullscreenStateChanged(). We don't do this immediately as
    // BrowserWindow::SetFullscreen() asks for bookmark_bar_state_, so we let the
    // BrowserWindow invoke WindowFullscreenStateChanged when appropriate.

    if(!entering_fullscreen)
    {
        NotifyTabOfFullscreenExitIfNecessary();
    }
}

void Browser::NotifyTabOfFullscreenExitIfNecessary()
{
    if(fullscreened_tab_)
    {
        fullscreened_tab_->ExitFullscreenMode();
    }
    fullscreened_tab_ = NULL;
    tab_caused_fullscreen_ = false;
}

void Browser::Exit()
{
    BrowserList::AttemptUserExit();
}

void Browser::BookmarkCurrentPage()
{
    //BookmarkModel* model = profile()->GetBookmarkModel();
    //if(!model || !model->IsLoaded())
    //{
    //    return; // Ignore requests until bookmarks are loaded.
    //}

    //GURL url;
    //string16 title;
    //TabContentsWrapper* tab = GetSelectedTabContentsWrapper();
    //bookmark_utils::GetURLAndTitleToBookmark(tab->tab_contents(), &url, &title);
    //bool was_bookmarked = model->IsBookmarked(url);
    //if(!was_bookmarked && profile_->IsOffTheRecord())
    //{
    //    // If we're incognito the favicon may not have been saved. Save it now
    //    // so that bookmarks have an icon for the page.
    //    tab->favicon_tab_helper()->SaveFavicon();
    //}
    //bookmark_utils::AddIfNotBookmarked(model, url, title);
    //// Make sure the model actually added a bookmark before showing the star. A
    //// bookmark isn't created if the url is invalid.
    //if(window_->IsActive() && model->IsBookmarked(url))
    //{
    //    // Only show the bubble if the window is active, otherwise we may get into
    //    // weird situations were the bubble is deleted as soon as it is shown.
    //    window_->ShowBookmarkBubble(url, was_bookmarked);
    //}
}

void Browser::SavePage()
{
    //TabContents* current_tab = GetSelectedTabContents();
    //if(current_tab && current_tab->contents_mime_type()=="application/pdf")
    //{
    //    UserMetrics::RecordAction(UserMetricsAction("PDF.SavePage"));
    //}
    //GetSelectedTabContents()->OnSavePage();
}

bool Browser::SupportsWindowFeature(WindowFeature feature) const
{
    return SupportsWindowFeatureImpl(feature, true);
}

bool Browser::CanSupportWindowFeature(WindowFeature feature) const
{
    return SupportsWindowFeatureImpl(feature, false);
}

void Browser::Cut()
{
    window()->Cut();
}

void Browser::Copy()
{
    window()->Copy();
}

void Browser::Paste()
{
    window()->Paste();
}

void Browser::Find()
{
    //FindInPage(false, false);
}

void Browser::FindNext()
{
    //FindInPage(true, true);
}

void Browser::FindPrevious()
{
    //FindInPage(true, false);
}

void Browser::FocusToolbar()
{
    window_->FocusToolbar();
}

void Browser::FocusAppMenu()
{
    window_->FocusAppMenu();
}

void Browser::FocusLocationBar()
{
    window_->SetFocusToLocationBar(true);
}

void Browser::FocusBookmarksToolbar()
{
    window_->FocusBookmarksToolbar();
}

void Browser::FocusNextPane()
{
    window_->RotatePaneFocus(true);
}

void Browser::FocusPreviousPane()
{
    window_->RotatePaneFocus(false);
}

void Browser::FocusSearch()
{
    window_->GetLocationBar()->FocusSearch();
}

void Browser::OpenTaskManager(bool highlight_background_resources)
{
    if(highlight_background_resources)
    {
        window_->ShowBackgroundPages();
    }
    else
    {
        window_->ShowTaskManager();
    }
}

void Browser::OpenBugReportDialog()
{
    //browser::ShowHtmlBugReportView(this, std::string(), 0);
}

void Browser::ToggleBookmarkBar()
{
    window_->ToggleBookmarkBar();
}

void Browser::OpenBookmarkManager()
{
    //ShowSingletonTabOverwritingNTP(
    //  GetSingletonTabNavigateParams(GURL(chrome::kChromeUIBookmarksURL)));
}

void Browser::OpenBookmarkManagerWithHash(const std::string& action,
                                          int64 node_id)
{
    //browser::NavigateParams params(GetSingletonTabNavigateParams(
    //    GURL(chrome::kChromeUIBookmarksURL).Resolve(
    //    StringPrintf("/#%s%s", action.c_str(),
    //    base::Int64ToString(node_id).c_str()))));
    //params.path_behavior = browser::NavigateParams::IGNORE_AND_NAVIGATE;
    //ShowSingletonTabOverwritingNTP(params);
}

void Browser::OpenBookmarkManagerForNode(int64 node_id)
{
    OpenBookmarkManagerWithHash("", node_id);
}

void Browser::OpenBookmarkManagerEditNode(int64 node_id)
{
    OpenBookmarkManagerWithHash("e=", node_id);
}

void Browser::OpenBookmarkManagerAddNodeIn(int64 node_id)
{
    OpenBookmarkManagerWithHash("a=", node_id);
}

void Browser::ShowAppMenu()
{
    // We record the user metric for this event in WrenchMenu::RunMenu.
    window_->ShowAppMenu();
}

void Browser::ShowHistoryTab()
{
    //ShowSingletonTabOverwritingNTP(
    //    GetSingletonTabNavigateParams(GURL(chrome::kChromeUIHistoryURL)));
}

void Browser::ShowAboutConflictsTab()
{
    //ShowSingletonTab(GURL(chrome::kChromeUIConflictsURL));
}

void Browser::ShowBrokenPageTab(TabContents* contents)
{
    //string16 page_title = contents->GetTitle();
    //NavigationEntry* entry = contents->controller().GetActiveEntry();
    //if(!entry)
    //{
    //    return;
    //}
    //std::string page_url = entry->url().spec();
    //std::vector<std::string> subst;
    //subst.push_back(UTF16ToASCII(page_title));
    //subst.push_back(page_url);
    //std::string report_page_url =
    //    ReplaceStringPlaceholders(kBrokenPageUrl, subst, NULL);
    //ShowSingletonTab(GURL(report_page_url));
}

void Browser::ShowOptionsTab(const std::string& sub_page)
{
    //browser::NavigateParams params(GetSingletonTabNavigateParams(
    //    GURL(chrome::kChromeUISettingsURL + sub_page)));
    //params.path_behavior = browser::NavigateParams::IGNORE_AND_NAVIGATE;

    //ShowSingletonTabOverwritingNTP(params);
}

void Browser::OpenClearBrowsingDataDialog()
{
    //ShowOptionsTab(chrome::kClearBrowserDataSubPage);
}

void Browser::OpenOptionsDialog()
{
    ShowOptionsTab("");
}

void Browser::OpenPasswordManager()
{
    //ShowOptionsTab(chrome::kPasswordManagerSubPage);
}

void Browser::OpenImportSettingsDialog()
{
    //ShowOptionsTab(chrome::kImportDataSubPage);
}

void Browser::OpenAboutChromeDialog()
{
    window_->ShowAboutChromeDialog();
}

void Browser::OpenUpdateChromeDialog()
{
    window_->ShowUpdateChromeDialog();
}

void Browser::ShowHelpTab()
{
    //GURL help_url(kHelpContentUrl);
    //GURL localized_help_url = google_util::AppendGoogleLocaleParam(help_url);
    //ShowSingletonTab(localized_help_url);
}

void Browser::OpenPrivacyDashboardTabAndActivate()
{
    //OpenURL(GURL(kPrivacyDashboardUrl), GURL(),
    //        NEW_FOREGROUND_TAB, PageTransition::LINK);
    //window_->Activate();
}

void Browser::OpenAutofillHelpTabAndActivate()
{
    //GURL help_url = google_util::AppendGoogleLocaleParam(GURL(kAutofillHelpUrl));
    //AddSelectedTabWithURL(help_url, PageTransition::LINK);
}

void Browser::OpenSearchEngineOptionsDialog()
{
    //UserMetrics::RecordAction(UserMetricsAction("EditSearchEngines"));
    //ShowOptionsTab(chrome::kSearchEnginesSubPage);
}

void Browser::OpenPluginsTabAndActivate()
{
    //OpenURL(GURL(chrome::kChromeUIPluginsURL), GURL(),
    //        NEW_FOREGROUND_TAB, PageTransition::LINK);
    //window_->Activate();
}

///////////////////////////////////////////////////////////////////////////////

// static
bool Browser::RunUnloadEventsHelper(TabContents* contents)
{
    // If the TabContents is not connected yet, then there's no unload
    // handler we can fire even if the TabContents has an unload listener.
    // One case where we hit this is in a tab that has an infinite loop
    // before load.
    if(contents->NeedToFireBeforeUnload())
    {
        // If the page has unload listeners, then we tell the renderer to fire
        // them. Once they have fired, we'll get a message back saying whether
        // to proceed closing the page or not, which sends us back to this method
        // with the NeedToFireBeforeUnload bit cleared.
        //contents->render_view_host()->FirePageBeforeUnload(false);
        return true;
    }
    return false;
}

void Browser::ExecuteCommandWithDisposition(
    int id, WindowOpenDisposition disposition)
{
    // No commands are enabled if there is not yet any selected tab.
    // TODO(pkasting): It seems like we should not need this, because either
    // most/all commands should not have been enabled yet anyway or the ones that
    // are enabled should be global, or safe themselves against having no selected
    // tab.  However, Ben says he tried removing this before and got lots of
    // crashes, e.g. from Windows sending WM_COMMANDs at random times during
    // window construction.  This probably could use closer examination someday.
    if(!GetSelectedTabContentsWrapper())
    {
        return;
    }

    //DCHECK(command_updater_.IsCommandEnabled(id)) << "Invalid/disabled command "
    //    << id;

    //// If command execution is blocked then just record the command and return.
    //if(block_command_execution_)
    //{
    //    // We actually only allow no more than one blocked command, otherwise some
    //    // commands maybe lost.
    //    DCHECK_EQ(last_blocked_command_id_, -1);
    //    last_blocked_command_id_ = id;
    //    last_blocked_command_disposition_ = disposition;
    //    return;
    //}

    //// The order of commands in this switch statement must match the function
    //// declaration order in browser.h!
	TabContents* tab = GetSelectedTabContents();
    switch(id)
    {
    //    // Navigation commands
    //case IDC_BACK:                  GoBack(disposition);              break;
    //case IDC_FORWARD:               GoForward(disposition);           break;
    case IDC_RELOAD:                if (tab) tab->do_restart();              break;
    //case IDC_RELOAD_IGNORING_CACHE: ReloadIgnoringCache(disposition); break;
    //case IDC_HOME:                  Home(disposition);                break;
    //case IDC_OPEN_CURRENT_URL:      OpenCurrentURL();                 break;
    //case IDC_STOP:                  Stop();                           break;

    //    // Window management commands
    //case IDC_NEW_WINDOW:            NewWindow();                      break;
    //case IDC_NEW_INCOGNITO_WINDOW:  NewIncognitoWindow();             break;
    //case IDC_CLOSE_WINDOW:          CloseWindow();                    break;
    //case IDC_NEW_TAB:               NewTab();                         break;
    //case IDC_CLOSE_TAB:             CloseTab();                       break;
    //case IDC_SELECT_NEXT_TAB:       SelectNextTab();                  break;
    //case IDC_SELECT_PREVIOUS_TAB:   SelectPreviousTab();              break;
    //case IDC_TABPOSE:               OpenTabpose();                    break;
    //case IDC_MOVE_TAB_NEXT:         MoveTabNext();                    break;
    //case IDC_MOVE_TAB_PREVIOUS:     MoveTabPrevious();                break;
    //case IDC_SELECT_TAB_0:
    //case IDC_SELECT_TAB_1:
    //case IDC_SELECT_TAB_2:
    //case IDC_SELECT_TAB_3:
    //case IDC_SELECT_TAB_4:
    //case IDC_SELECT_TAB_5:
    //case IDC_SELECT_TAB_6:
    //case IDC_SELECT_TAB_7:          SelectNumberedTab(id - IDC_SELECT_TAB_0);
    //    break;
    //case IDC_SELECT_LAST_TAB:       SelectLastTab();                  break;
    case IDC_DUPLICATE_TAB:			DuplicateCurrentTab();                   break;
    //case IDC_RESTORE_TAB:           RestoreTab();                     break;
    //case IDC_COPY_URL:              WriteCurrentURLToClipboard();     break;
    //case IDC_SHOW_AS_TAB:           ConvertPopupToTabbedBrowser();    break;
    //case IDC_FULLSCREEN:            ToggleFullscreenMode();           break;
    //case IDC_EXIT:                  Exit();                           break;
    //case IDC_TOGGLE_VERTICAL_TABS:  ToggleUseVerticalTabs();          break;
    //case IDC_COMPACT_NAVBAR:        ToggleUseCompactNavigationBar();  break;

    //    // Clipboard commands
    //case IDC_CUT:                   Cut();                            break;
    //case IDC_COPY:                  Copy();                           break;
    case IDC_PASTE:                 if (tab) tab->do_paste();                          break;

    //    // Find-in-page
    //case IDC_FIND:                  Find();                           break;
    //case IDC_FIND_NEXT:             FindNext();                       break;
    //case IDC_FIND_PREVIOUS:         FindPrevious();                   break;

    //    // Focus various bits of UI
    //case IDC_FOCUS_TOOLBAR:         FocusToolbar();                   break;
    //case IDC_FOCUS_LOCATION:        FocusLocationBar();               break;
    //case IDC_FOCUS_SEARCH:          FocusSearch();                    break;
    //case IDC_FOCUS_MENU_BAR:        FocusAppMenu();                   break;
    //case IDC_FOCUS_BOOKMARKS:       FocusBookmarksToolbar();          break;
    //case IDC_FOCUS_CHROMEOS_STATUS: FocusChromeOSStatus();            break;
    //case IDC_FOCUS_NEXT_PANE:       FocusNextPane();                  break;
    //case IDC_FOCUS_PREVIOUS_PANE:   FocusPreviousPane();              break;

    //    // Show various bits of UI
    //case IDC_TASK_MANAGER:          OpenTaskManager(false);           break;
    //case IDC_VIEW_BACKGROUND_PAGES: OpenTaskManager(true);            break;
    //case IDC_FEEDBACK:              OpenBugReportDialog();            break;

    //case IDC_SHOW_BOOKMARK_BAR:     ToggleBookmarkBar();              break;
    //case IDC_PROFILING_ENABLED:     Profiling::Toggle();              break;

    //case IDC_SHOW_BOOKMARK_MANAGER: OpenBookmarkManager();            break;
    //case IDC_SHOW_APP_MENU:         ShowAppMenu();                    break;
    //case IDC_SHOW_HISTORY:          ShowHistoryTab();                 break;
    //case IDC_SHOW_DOWNLOADS:        ShowDownloadsTab();               break;
    //case IDC_MANAGE_EXTENSIONS:     ShowExtensionsTab();              break;
    //case IDC_OPTIONS:               OpenOptionsDialog();              break;
    //case IDC_EDIT_SEARCH_ENGINES:   OpenSearchEngineOptionsDialog();  break;
    //case IDC_VIEW_PASSWORDS:        OpenPasswordManager();            break;
    //case IDC_CLEAR_BROWSING_DATA:   OpenClearBrowsingDataDialog();    break;
    //case IDC_IMPORT_SETTINGS:       OpenImportSettingsDialog();       break;
    //case IDC_ABOUT:                 OpenAboutChromeDialog();          break;
    //case IDC_UPGRADE_DIALOG:        OpenUpdateChromeDialog();         break;
    //case IDC_VIEW_INCOMPATIBILITIES: ShowAboutConflictsTab();         break;
    //case IDC_HELP_PAGE:             ShowHelpTab();                    break;

    default:
        LOG(WARNING) << "Received Unimplemented Command: " << id;
        break;
    }
}

bool Browser::ExecuteCommandIfEnabled(int id)
{
    //if(command_updater_.SupportsCommand(id) &&
    //    command_updater_.IsCommandEnabled(id))
    //{
    //    ExecuteCommand(id);
    //    return true;
    //}
    return false;
}

void Browser::SetBlockCommandExecution(bool block)
{
    block_command_execution_ = block;
    if(block)
    {
        last_blocked_command_id_ = -1;
        last_blocked_command_disposition_ = CURRENT_TAB;
    }
}

int Browser::GetLastBlockedCommand(WindowOpenDisposition* disposition)
{
    if(disposition)
    {
        *disposition = last_blocked_command_disposition_;
    }
    return last_blocked_command_id_;
}

void Browser::UpdateUIForNavigationInTab(TabContentsWrapper* contents,
                                         bool user_initiated)
{
    tabstrip_model()->TabNavigating(contents);

    bool contents_is_selected = contents == GetSelectedTabContentsWrapper();
    if(user_initiated && contents_is_selected && window()->GetLocationBar())
    {
        // Forcibly reset the location bar if the url is going to change in the
        // current tab, since otherwise it won't discard any ongoing user edits,
        // since it doesn't realize this is a user-initiated action.
        window()->GetLocationBar()->Revert();
    }

    // Update the location bar. This is synchronous. We specifically don't
    // update the load state since the load hasn't started yet and updating it
    // will put it out of sync with the actual state like whether we're
    // displaying a favicon, which controls the throbber. If we updated it here,
    // the throbber will show the default favicon for a split second when
    // navigating away from the new tab page.
    ScheduleUIUpdate(contents->tab_contents(), TabContents::INVALIDATE_URL);

    if(contents_is_selected)
    {
        contents->tab_contents()->Focus();
    }
}

Url Browser::GetHomePage() const
{
    // --homepage overrides any preferences.
    //const CommandLine& command_line = *CommandLine::ForCurrentProcess();
    //if(command_line.HasSwitch(switches::kHomePage))
    //{
    //    // TODO(evanm): clean up usage of DIR_CURRENT.
    //    //   http://code.google.com/p/chromium/issues/detail?id=60630
    //    // For now, allow this code to call getcwd().
    //    base::ThreadRestrictions::ScopedAllowIO allow_io;

    //    FilePath browser_directory;
    //    PathService::Get(base::DIR_CURRENT, &browser_directory);
    //    GURL home_page(URLFixerUpper::FixupRelativeFile(browser_directory,
    //        command_line.GetSwitchValuePath(switches::kHomePage)));
    //    if(home_page.is_valid())
    //    {
    //        return home_page;
    //    }
    //}

    //if(profile_->GetPrefs()->GetBoolean(prefs::kHomePageIsNewTabPage))
    //{
    //    return GURL(chrome::kChromeUINewTabURL);
    //}
    //GURL home_page(URLFixerUpper::FixupURL(
    //    profile_->GetPrefs()->GetString(prefs::kHomePage),
    //    std::string()));
    //if(!home_page.is_valid())
    //{
    //    return GURL(chrome::kChromeUINewTabURL);
    //}
    //return home_page;
    return Url();
}

///////////////////////////////////////////////////////////////////////////////
// Browser, PageNavigator implementation:

// TODO(adriansc): Remove this method once refactoring changed all call sites.
TabContents* Browser::OpenURL(const Url& url,
                              const Url& referrer,
                              WindowOpenDisposition disposition)
{
    return OpenURLFromTab(NULL,
        OpenURLParams(url, referrer, disposition));
}

TabContents* Browser::OpenURL(const OpenURLParams& params)
{
    return OpenURLFromTab(NULL, params);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, CommandUpdater::CommandUpdaterDelegate implementation:

void Browser::ExecuteCommand(int id)
{
    ExecuteCommandWithDisposition(id, CURRENT_TAB);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, TabHandlerDelegate implementation:

Browser* Browser::AsBrowser()
{
    return this;
}

///////////////////////////////////////////////////////////////////////////////
// Browser, TabStripModelDelegate implementation:

TabContentsWrapper* Browser::AddBlankTab(bool foreground)
{
    return AddBlankTabAt(-1, foreground);
}

TabContentsWrapper* Browser::AddBlankTabAt(int index, bool foreground)
{
    // Time new tab page creation time.  We keep track of the timing data in
    // TabContents, but we want to include the time it takes to create the
    // TabContents object too.
    base::TimeTicks new_tab_start_time = base::TimeTicks::Now();
    browser::NavigateParams params(this, Url::EmptyGURL()/*GURL(chrome::kChromeUINewTabURL)*/);
    params.disposition = foreground ? NEW_FOREGROUND_TAB : NEW_BACKGROUND_TAB;
    params.tabstrip_index = index;
    browser::Navigate(&params);
	if (NULL != params.target_contents){
		params.target_contents->tab_contents()->set_new_tab_start_time(
			new_tab_start_time);
	}
    return params.target_contents;
}

TabContentsWrapper*  Browser::DuplicateCurrentTab()
{
	base::TimeTicks new_tab_start_time = base::TimeTicks::Now();
    browser::NavigateParams params(this, Url::EmptyGURL()/*GURL(chrome::kChromeUINewTabURL)*/);
    params.disposition = NEW_FOREGROUND_TAB;
	params.isDuplicateSourceContent = true;
    params.tabstrip_index = active_index() + 1;
    browser::Navigate(&params);
	if (NULL != params.target_contents){
		params.target_contents->tab_contents()->set_new_tab_start_time(
			new_tab_start_time);
	}
    return params.target_contents;
}

Browser* Browser::CreateNewStripWithContents(
    TabContentsWrapper* detached_contents,
    const gfx::Rect& window_bounds,
    const DockInfo& dock_info,
    bool maximize)
{
    DCHECK(CanSupportWindowFeature(FEATURE_TABSTRIP));

    gfx::Rect new_window_bounds = window_bounds;
    if(dock_info.GetNewWindowBounds(&new_window_bounds, &maximize))
    {
        dock_info.AdjustOtherWindowBounds();
    }

    // Create an empty new browser window the same size as the old one.
    Browser* browser = new Browser(TYPE_TABBED);
    browser->set_override_bounds(new_window_bounds);
    browser->set_show_state(
        maximize ? ui::SHOW_STATE_MAXIMIZED : ui::SHOW_STATE_NORMAL);
    browser->InitBrowserWindow();
    browser->tabstrip_model()->AppendTabContents(detached_contents, true);
    // Make sure the loading state is updated correctly, otherwise the throbber
    // won't start if the page is loading.
    browser->LoadingStateChanged(detached_contents->tab_contents());
    return browser;
}

int Browser::GetDragActions() const
{
    return TabStripModelDelegate::TAB_TEAROFF_ACTION | (tab_count() > 1 ?
        TabStripModelDelegate::TAB_MOVE_ACTION : 0);
}

TabContentsWrapper* Browser::CreateTabContentsForURL(
    const Url& url, const Url& referrer, bool defer_load) const
{
    TabContentsWrapper* contents = TabContentsFactory(
        -2/*MSG_ROUTING_NONE*/,
        GetSelectedTabContents(),
		false);
    if(!defer_load)
    {
        // Load the initial URL before adding the new tab contents to the tab strip
        // so that the tab contents has navigation state.
        //contents->controller().LoadURL(url, referrer, transition);
    }

    return contents;
}

bool Browser::CanDuplicateContentsAt(int index)
{
    return true;
    //NavigationController& nc = GetTabContentsAt(index)->controller();
    //return nc.tab_contents() && nc.GetLastCommittedEntry();
}

void Browser::DuplicateContentsAt(int index)
{
    TabContentsWrapper* contents = GetTabContentsWrapperAt(index);
    CHECK(contents);
    TabContentsWrapper* contents_dupe = contents->Clone();

    bool pinned = false;
    if(CanSupportWindowFeature(FEATURE_TABSTRIP))
    {
        // If this is a tabbed browser, just create a duplicate tab inside the same
        // window next to the tab being duplicated.
        int index = tab_handler_->GetTabStripModel()->
            GetIndexOfTabContents(contents);
        int add_types = TabStripModel::ADD_ACTIVE |
            TabStripModel::ADD_INHERIT_GROUP;
        tab_handler_->GetTabStripModel()->InsertTabContentsAt(index + 1,
            contents_dupe,
            add_types);
    }
    else
    {
        Browser* browser = NULL;
        if(is_type_popup())
        {
            browser = Browser::CreateForType(TYPE_POPUP);
        }

        // Preserve the size of the original window. The new window has already
        // been given an offset by the OS, so we shouldn't copy the old bounds.
        BrowserWindow* new_window = browser->window();
        new_window->SetBounds(gfx::Rect(new_window->GetRestoredBounds().origin(),
            window()->GetRestoredBounds().size()));

        // We need to show the browser now.  Otherwise ContainerWin assumes the
        // TabContents is invisible and won't size it.
        browser->window()->Show();

        // The page transition below is only for the purpose of inserting the tab.
        browser->AddTab(contents_dupe);
    }

    //SessionService* session_service =
    //    SessionServiceFactory::GetForProfileIfExisting(profile_);
    //if(session_service)
    //{
    //    session_service->TabRestored(contents_dupe, pinned);
    //}
}

void Browser::CloseFrameAfterDragSession()
{
    // This is scheduled to run after we return to the message loop because
    // otherwise the frame will think the drag session is still active and ignore
    // the request.
    // TODO(port): figure out what is required here in a cross-platform world
    MessageLoop::current()->PostTask(
        method_factory_.NewRunnableMethod(&Browser::CloseFrame));
}

void Browser::CreateHistoricalTab(TabContentsWrapper* contents)
{
    // We don't create historical tabs for incognito windows or windows without
    // profiles.
    //if(!profile() || profile()->IsOffTheRecord())
    //{
    //    return;
    //}

    //// We don't create historical tabs for print preview tabs.
    //if(contents->tab_contents()->GetURL() == GURL(chrome::kChromeUIPrintURL))
    //{
    //    return;
    //}

    //TabRestoreService* service =
    //    TabRestoreServiceFactory::GetForProfile(profile());

    //// We only create historical tab entries for tabbed browser windows.
    //if(service && CanSupportWindowFeature(FEATURE_TABSTRIP))
    //{
    //    service->CreateHistoricalTab(&contents->controller(),
    //        tab_handler_->GetTabStripModel()->GetIndexOfTabContents(contents));
    //}
}

bool Browser::RunUnloadListenerBeforeClosing(TabContentsWrapper* contents)
{
    return Browser::RunUnloadEventsHelper(contents->tab_contents());
}

bool Browser::CanReloadContents(TabContents* source) const
{
    return true;
}

bool Browser::CanCloseContents(std::vector<int>* indices)
{
    DCHECK(!indices->empty());
    //TabCloseableStateWatcher* watcher =
    //    g_browser_process->tab_closeable_state_watcher();
    //bool can_close_all = !watcher || watcher->CanCloseTabs(this, indices);
    //if(indices->empty()) // Cannot close any tab.
    //{
    //    return false;
    //}
    //// Now, handle cases where at least one tab can be closed.
    //// If we are closing all the tabs for this browser, make sure to check for
    //// in-progress downloads.
    //// Note that the next call when it returns false will ask the user for
    //// confirmation before closing the browser if the user decides so.
    //if(tab_handler_->GetTabStripModel()->count() ==
    //    static_cast<int>(indices->size()) &&
    //    !CanCloseWithInProgressDownloads())
    //{
    //    indices->clear();
    //    can_close_all = false;
    //}
    //return can_close_all;
    return true;
}

bool Browser::CanBookmarkAllTabs() const
{
    return true;
    //BookmarkModel* model = profile()->GetBookmarkModel();
    //return (model && model->IsLoaded()) &&
    //    tab_count() > 1 &&
    //    profile()->GetPrefs()->GetBoolean(prefs::kEditBookmarksEnabled);
}

void Browser::BookmarkAllTabs()
{
    //BookmarkModel* model = profile()->GetBookmarkModel();
    //DCHECK(model && model->IsLoaded());

    //BookmarkEditor::EditDetails details;
    //details.type = BookmarkEditor::EditDetails::NEW_FOLDER;
    //bookmark_utils::GetURLsForOpenTabs(this, &(details.urls));
    //DCHECK(!details.urls.empty());

    //BookmarkEditor::Show(window()->GetNativeHandle(), profile_,
    //    model->GetParentForNewNodes(),  details,
    //    BookmarkEditor::SHOW_TREE);
}

bool Browser::CanCloseTab() const
{
    //TabCloseableStateWatcher* watcher =
    //    g_browser_process->tab_closeable_state_watcher();
    //return !watcher || watcher->CanCloseTab(this);
    return true;
}

bool Browser::LargeIconsPermitted() const
{
    // We don't show the big icons in tabs for TYPE_EXTENSION_APP windows because
    // for those windows, we already have a big icon in the top-left outside any
    // tab. Having big tab icons too looks kinda redonk.
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Browser, TabStripModelObserver implementation:

void Browser::TabInsertedAt(TabContentsWrapper* contents,
                            int index,
                            bool foreground)
{
    SetAsDelegate(contents, this);
    //contents->restore_tab_helper()->SetWindowID(session_id());

    //SyncHistoryWithTabs(index);

    // Make sure the loading state is updated correctly, otherwise the throbber
    // won't start if the page is loading.
    LoadingStateChanged(contents->tab_contents());
}

void Browser::TabClosingAt(TabStripModel* tab_strip_model,
                           TabContentsWrapper* contents,
                           int index)
{
    // Sever the TabContents' connection back to us.
    SetAsDelegate(contents, NULL);
}

void Browser::TabDetachedAt(TabContentsWrapper* contents, int index)
{
    TabDetachedAtImpl(contents, index, DETACH_TYPE_DETACH);
}

void Browser::TabDeactivated(TabContentsWrapper* contents)
{
    if(contents == fullscreened_tab_)
    {
        ExitTabbedFullscreenModeIfNecessary();
    }

    // Save what the user's currently typing, so it can be restored when we
    // switch back to this tab.
    window_->GetLocationBar()->SaveStateToContents(contents->tab_contents());
}

void Browser::ActiveTabChanged(TabContentsWrapper* old_contents,
                               TabContentsWrapper* new_contents,
                               int index,
                               bool user_gesture)
{
    // On some platforms we want to automatically reload tabs that are
    // killed when the user selects them.
    if(user_gesture && new_contents->tab_contents()->crashed_status() ==
        base::TERMINATION_STATUS_PROCESS_WAS_KILLED)
    {
        //const CommandLine& parsed_command_line = *CommandLine::ForCurrentProcess();
        //if(parsed_command_line.HasSwitch(switches::kReloadKilledTabs))
        //{
        //    Reload(CURRENT_TAB);
        //    return;
        //}
    }

    // If we have any update pending, do it now.
    if(!chrome_updater_factory_.empty() && old_contents)
    {
        ProcessPendingUIUpdates();
    }

    // Propagate the profile to the location bar.
    UpdateToolbar(true);

    // Update reload/stop state.
    UpdateReloadStopState(new_contents->tab_contents()->IsLoading(), true);

    // Update commands to reflect current state.
    UpdateCommandsForTabState();

    UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_TAB_SWITCH);
}

void Browser::TabMoved(TabContentsWrapper* contents,
                       int from_index,
                       int to_index)
{
    DCHECK(from_index >= 0 && to_index >= 0);
    // Notify the history service.
    //SyncHistoryWithTabs(std::min(from_index, to_index));
}

void Browser::TabReplacedAt(TabStripModel* tab_strip_model,
                            TabContentsWrapper* old_contents,
                            TabContentsWrapper* new_contents,
                            int index)
{
    TabDetachedAtImpl(old_contents, index, DETACH_TYPE_REPLACE);
    TabInsertedAt(new_contents, index,
        (index == tab_handler_->GetTabStripModel()->active_index()));

    //int entry_count = new_contents->controller().entry_count();
    //if(entry_count > 0)
    //{
    //    // Send out notification so that observers are updated appropriately.
    //    new_contents->controller().NotifyEntryChanged(
    //        new_contents->controller().GetEntryAtIndex(entry_count - 1),
    //        entry_count - 1);
    //}

    //SessionService* session_service =
    //    SessionServiceFactory::GetForProfile(profile());
    //if(session_service)
    //{
    //    // The new_contents may end up with a different navigation stack. Force
    //    // the session service to update itself.
    //    session_service->TabRestored(
    //        new_contents, tab_handler_->GetTabStripModel()->IsTabPinned(index));
    //}
}

void Browser::TabStripEmpty()
{
    // Close the frame after we return to the message loop (not immediately,
    // otherwise it will destroy this object before the stack has a chance to
    // cleanly unwind.)
    // Note: This will be called several times if TabStripEmpty is called several
    //       times. This is because it does not close the window if tabs are
    //       still present.
    // NOTE: If you change to be immediate (no invokeLater) then you'll need to
    //       update BrowserList::CloseAllBrowsers.
    MessageLoop::current()->PostTask(
        method_factory_.NewRunnableMethod(&Browser::CloseFrame));
}

///////////////////////////////////////////////////////////////////////////////
// Browser, TabContentsDelegate implementation:

// TODO(adriansc): Remove this method once refactoring changed all call sites.
TabContents* Browser::OpenURLFromTab(TabContents* source,
                                     const Url& url,
                                     const Url& referrer,
                                     WindowOpenDisposition disposition)
{
    return OpenURLFromTab(source, OpenURLParams(url, referrer, disposition));
}

TabContents* Browser::OpenURLFromTab(TabContents* source,
                                     const OpenURLParams& params)
{
    browser::NavigateParams nav_params(this, params.url);
    nav_params.source_contents =
        tabstrip_model()->GetTabContentsAt(
        tabstrip_model()->GetWrapperIndex(source));
    nav_params.referrer = params.referrer;
    nav_params.disposition = params.disposition;
    nav_params.tabstrip_add_types = TabStripModel::ADD_NONE;
    nav_params.window_action = browser::NavigateParams::SHOW_WINDOW;
    nav_params.user_gesture = true;
    browser::Navigate(&nav_params);

    return nav_params.target_contents ?
        nav_params.target_contents->tab_contents() : NULL;
}

void Browser::NavigationStateChanged(const TabContents* source,
                                     unsigned changed_flags)
{
    // Only update the UI when something visible has changed.
    if(changed_flags)
    {
        ScheduleUIUpdate(source, changed_flags);
    }

    // We don't schedule updates to commands since they will only change once per
    // navigation, so we don't have to worry about flickering.
    if(changed_flags & TabContents::INVALIDATE_URL)
    {
        UpdateCommandsForTabState();
    }
}

void Browser::AddNewContents(TabContents* source,
                             TabContents* new_contents,
                             WindowOpenDisposition disposition,
                             const gfx::Rect& initial_pos,
                             bool user_gesture)
{
    // No code for this yet.
    DCHECK(disposition != SAVE_TO_DISK);
    // Can't create a new contents for the current tab - invalid case.
    DCHECK(disposition != CURRENT_TAB);

    //TabContentsWrapper* source_wrapper = NULL;
    //BlockedContentTabHelper* source_blocked_content = NULL;
    //TabContentsWrapper* new_wrapper =
    //    TabContentsWrapper::GetCurrentWrapperForContents(new_contents);
    //if(!new_wrapper)
    //{
    //    new_wrapper = new TabContentsWrapper(new_contents);
    //}
    //if(source)
    //{
    //    source_wrapper = TabContentsWrapper::GetCurrentWrapperForContents(source);
    //    source_blocked_content = source_wrapper->blocked_content_tab_helper();
    //}

    //if(source_wrapper)
    //{
    //    // Handle blocking of all contents.
    //    if(source_blocked_content->all_contents_blocked())
    //    {
    //        source_blocked_content->AddTabContents(new_wrapper,
    //            disposition,
    //            initial_pos,
    //            user_gesture);
    //        return;
    //    }

    //    // Handle blocking of popups.
    //    if((disposition == NEW_POPUP) && !user_gesture &&
    //        !CommandLine::ForCurrentProcess()->HasSwitch(
    //        switches::kDisablePopupBlocking))
    //    {
    //        // Unrequested popups from normal pages are constrained unless they're in
    //        // the whitelist.  The popup owner will handle checking this.
    //        GetConstrainingContentsWrapper(source_wrapper)->
    //            blocked_content_tab_helper()->
    //            AddPopup(new_wrapper, initial_pos, user_gesture);
    //        return;
    //    }

    //    RenderViewHost* view = new_contents->render_view_host();
    //    view->Send(new ViewMsg_DisassociateFromPopupCount(view->routing_id()));
    //}

    //browser::NavigateParams params(this, new_wrapper);
    //params.source_contents =
    //    source ? tabstrip_model()->GetTabContentsAt(
    //    tabstrip_model()->GetWrapperIndex(source))
    //    : NULL;
    //params.disposition = disposition;
    //params.window_bounds = initial_pos;
    //params.window_action = browser::NavigateParams::SHOW_WINDOW;
    //params.user_gesture = user_gesture;
    //browser::Navigate(&params);

    //NotificationService::current()->Notify(
    //    content::NOTIFICATION_TAB_ADDED,
    //    Source<TabContentsDelegate>(this),
    //    Details<TabContents>(new_contents));
}

void Browser::ActivateContents(TabContents* contents)
{
    tab_handler_->GetTabStripModel()->ActivateTabAt(
        tab_handler_->GetTabStripModel()->GetWrapperIndex(contents), false);
    window_->Activate();
}

void Browser::DeactivateContents(TabContents* contents)
{
    window_->Deactivate();
}

void Browser::LoadingStateChanged(TabContents* source)
{
    window_->UpdateLoadingAnimations(
        tab_handler_->GetTabStripModel()->TabsAreLoading());
    window_->UpdateTitleBar();

    TabContents* selected_contents = GetSelectedTabContents();
    if(source == selected_contents)
    {
        bool is_loading = source->IsLoading();
        UpdateReloadStopState(is_loading, false);

        //if(!is_loading && pending_web_app_action_ == UPDATE_SHORTCUT)
        //{
        //    // Schedule a shortcut update when web application info is available if
        //    // last committed entry is not NULL. Last committed entry could be NULL
        //    // when an interstitial page is injected (e.g. bad https certificate,
        //    // malware site etc). When this happens, we abort the shortcut update.
        //    NavigationEntry* entry = source->controller().GetLastCommittedEntry();
        //    if(entry)
        //    {
        //        TabContentsWrapper::GetCurrentWrapperForContents(source)->
        //            extension_tab_helper()->GetApplicationInfo(entry->page_id());
        //    }
        //    else
        //    {
        //        pending_web_app_action_ = NONE;
        //    }
        //}
    }
}

void Browser::CloseContents(TabContents* source)
{
    if(is_attempting_to_close_browser_)
    {
        // If we're trying to close the browser, just clear the state related to
        // waiting for unload to fire. Don't actually try to close the tab as it
        // will go down the slow shutdown path instead of the fast path of killing
        // all the renderer processes.
        ClearUnloadState(source, true);
        return;
    }

    int index = tab_handler_->GetTabStripModel()->GetWrapperIndex(source);
    if(index == TabStripModel::kNoTab)
    {
        NOTREACHED() << "CloseContents called for tab not in our strip";
        return;
    }
    tab_handler_->GetTabStripModel()->CloseTabContentsAt(
        index,
        TabStripModel::CLOSE_CREATE_HISTORICAL_TAB);
}

void Browser::MoveContents(TabContents* source, const gfx::Rect& pos)
{
    if(!IsPopupOrPanel(source))
    {
        NOTREACHED() << "moving invalid browser type";
        return;
    }
    window_->SetBounds(pos);
}

void Browser::DetachContents(TabContents* source)
{
    int index = tab_handler_->GetTabStripModel()->GetWrapperIndex(source);
    if(index >= 0)
    {
        tab_handler_->GetTabStripModel()->DetachTabContentsAt(index);
    }
}

bool Browser::IsPopupOrPanel(const TabContents* source) const
{
    // A non-tabbed BROWSER is an unconstrained popup.
    return is_type_popup() || is_type_panel();
}

void Browser::ContentsMouseEvent(TabContents* source,
                                 const gfx::Point& location,
                                 bool motion)
{
    //if(!GetStatusBubble())
    //{
    //    return;
    //}

    //if(source == GetSelectedTabContents())
    //{
    //    GetStatusBubble()->MouseMoved(location, !motion);
    //    if(!motion)
    //    {
    //        GetStatusBubble()->SetURL(GURL(), std::string());
    //    }
    //}
}

void Browser::UpdateTargetURL(TabContents* source, const Url& url)
{
    //if(!GetStatusBubble())
    //{
    //    return;
    //}

    //if(source == GetSelectedTabContents())
    //{
    //    PrefService* prefs = profile_->GetPrefs();
    //    GetStatusBubble()->SetURL(url, prefs->GetString(prefs::kAcceptLanguages));
    //}
}

void Browser::ContentsZoomChange(bool zoom_in)
{
    //ExecuteCommand(zoom_in ? IDC_ZOOM_PLUS : IDC_ZOOM_MINUS);
}

void Browser::SetTabContentBlocked(TabContents* contents, bool blocked)
{
    int index = tabstrip_model()->GetWrapperIndex(contents);
    if(index == TabStripModel::kNoTab)
    {
        NOTREACHED();
        return;
    }
    tabstrip_model()->SetTabBlocked(index, blocked);
}

void Browser::TabContentsFocused(TabContents* tab_content)
{
    window_->TabContentsFocused(tab_content);
}

bool Browser::TakeFocus(bool reverse)
{
    return false;
}

void Browser::BeforeUnloadFired(TabContents* tab,
                                bool proceed,
                                bool* proceed_to_fire_unload)
{
    if(!is_attempting_to_close_browser_)
    {
        *proceed_to_fire_unload = proceed;
        if(!proceed)
        {
            tab->set_closed_by_user_gesture(false);
        }
        return;
    }

    if(!proceed)
    {
        CancelWindowClose();
        *proceed_to_fire_unload = false;
        tab->set_closed_by_user_gesture(false);
        return;
    }

    if(RemoveFromSet(&tabs_needing_before_unload_fired_, tab))
    {
        // Now that beforeunload has fired, put the tab on the queue to fire
        // unload.
        tabs_needing_unload_fired_.insert(tab);
        ProcessPendingTabs();
        // We want to handle firing the unload event ourselves since we want to
        // fire all the beforeunload events before attempting to fire the unload
        // events should the user cancel closing the browser.
        *proceed_to_fire_unload = false;
        return;
    }

    *proceed_to_fire_unload = true;
}

void Browser::SetFocusToLocationBar(bool select_all)
{
    // Two differences between this and FocusLocationBar():
    // (1) This doesn't get recorded in user metrics, since it's called
    //     internally.
    // (2) This checks whether the location bar can be focused, and if not, clears
    //     the focus.  FocusLocationBar() is only reached when the location bar is
    //     focusable, but this may be reached at other times, e.g. while in
    //     fullscreen mode, where we need to leave focus in a consistent state.
    window_->SetFocusToLocationBar(select_all);
}

void Browser::RenderWidgetShowing()
{
    window_->DisableInactiveFrame();
}

int Browser::GetExtraRenderViewHeight() const
{
    return window_->GetExtraRenderViewHeight();
}

void Browser::ShowRepostFormWarningDialog(TabContents *tab_contents)
{
    window()->ShowRepostFormWarningDialog(tab_contents);
}

void Browser::ContentRestrictionsChanged(TabContents* source)
{
    UpdateCommandsForContentRestrictionState();
}

void Browser::RendererUnresponsive(TabContents* source)
{
    //browser::ShowHungRendererDialog(source);
}

void Browser::RendererResponsive(TabContents* source)
{
    //browser::HideHungRendererDialog(source);
}

void Browser::WorkerCrashed(TabContents* source)
{
    //TabContentsWrapper* wrapper =
    //    TabContentsWrapper::GetCurrentWrapperForContents(source);
    //wrapper->AddInfoBar(new SimpleAlertInfoBarDelegate(
    //    source, NULL, ui::GetStringUTF16(IDS_WEBWORKER_CRASHED_PROMPT),
    //    true));
}

void Browser::DidNavigateMainFramePostCommit(TabContents* tab)
{
    if(tab == GetSelectedTabContents())
    {
        UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_TAB_STATE);
    }
}

void Browser::DidNavigateToPendingEntry(TabContents* tab)
{
    if(tab == GetSelectedTabContents())
    {
        UpdateBookmarkBarState(BOOKMARK_BAR_STATE_CHANGE_TAB_STATE);
    }
}

void Browser::ToggleFullscreenModeForTab(TabContents* tab,
                                         bool enter_fullscreen)
{
    if(tab != GetSelectedTabContents())
    {
        return;
    }
    fullscreened_tab_ = enter_fullscreen ?
        TabContentsWrapper::GetCurrentWrapperForContents(tab) : NULL;
    if(enter_fullscreen && !window_->IsFullscreen())
    {
        tab_caused_fullscreen_ = true;
    }
    if(tab_caused_fullscreen_)
    {
        ToggleFullscreenMode();
    }
}

void Browser::ExitTabbedFullscreenModeIfNecessary()
{
    if(tab_caused_fullscreen_)
    {
        ToggleFullscreenMode();
    }
    else
    {
        NotifyTabOfFullscreenExitIfNecessary();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Browser, InstantDelegate implementation:

void Browser::SwapTabContents(TabContentsWrapper* old_tab_contents,
                              TabContentsWrapper* new_tab_contents)
{
    int index =
        tab_handler_->GetTabStripModel()->GetIndexOfTabContents(old_tab_contents);
    DCHECK_NE(TabStripModel::kNoTab, index);
    tab_handler_->GetTabStripModel()->ReplaceTabContentsAt(index, new_tab_contents);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, BlockedContentTabHelperDelegate implementation:

TabContentsWrapper* Browser::GetConstrainingContentsWrapper(
    TabContentsWrapper* source)
{
    return source;
}

///////////////////////////////////////////////////////////////////////////////
// Browser, BookmarkTabHelperDelegate implementation:

void Browser::URLStarredChanged(TabContentsWrapper* source, bool starred)
{
    if(source == GetSelectedTabContentsWrapper())
    {
        window_->SetStarredState(starred);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Browser, ProfileSyncServiceObserver implementation:

void Browser::OnStateChanged()
{
    const bool show_main_ui = is_type_tabbed() && !window_->IsFullscreen();
}

///////////////////////////////////////////////////////////////////////////////
// Browser, protected:

BrowserWindow* Browser::CreateBrowserWindow()
{
    //if(type_==TYPE_PANEL &&
    //    CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnablePanels))
    //{
    //    return PanelManager::GetInstance()->CreatePanel(this);
    //}

    return BrowserWindow::CreateBrowserWindow(this);
}


///////////////////////////////////////////////////////////////////////////////
// Browser, Command and state updating (private):

void Browser::InitCommandState()
{
    // All browser commands whose state isn't set automagically some other way
    // (like Back & Forward with initial page load) must have their state
    // initialized here, otherwise they will be forever disabled.

    // Navigation commands
    //command_updater_.UpdateCommandEnabled(IDC_RELOAD, true);
    //command_updater_.UpdateCommandEnabled(IDC_RELOAD_IGNORING_CACHE, true);

    //Window management commands
    //    IncognitoModePrefs::Availability incognito_avail =
    //    IncognitoModePrefs::GetAvailability(profile_->GetPrefs());
    //command_updater_.UpdateCommandEnabled(
    //    IDC_NEW_WINDOW,
    //    incognito_avail != IncognitoModePrefs::FORCED);
    //command_updater_.UpdateCommandEnabled(
    //    IDC_NEW_INCOGNITO_WINDOW,
    //    incognito_avail != IncognitoModePrefs::DISABLED);

    //command_updater_.UpdateCommandEnabled(IDC_CLOSE_WINDOW, true);
    //command_updater_.UpdateCommandEnabled(IDC_NEW_TAB, true);
    //command_updater_.UpdateCommandEnabled(IDC_CLOSE_TAB, true);
    //command_updater_.UpdateCommandEnabled(IDC_DUPLICATE_TAB, true);
    //command_updater_.UpdateCommandEnabled(IDC_RESTORE_TAB, false);
    //command_updater_.UpdateCommandEnabled(IDC_EXIT, true);
    //command_updater_.UpdateCommandEnabled(IDC_TOGGLE_VERTICAL_TABS, true);
    //command_updater_.UpdateCommandEnabled(IDC_DEBUG_FRAME_TOGGLE, true);

    //// Page-related commands
    //command_updater_.UpdateCommandEnabled(IDC_EMAIL_PAGE_LOCATION, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_AUTO_DETECT, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_UTF8, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_UTF16LE, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO88591, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_WINDOWS1252, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_GBK, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_GB18030, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_BIG5HKSCS, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_BIG5, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_THAI, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_KOREAN, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_SHIFTJIS, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO2022JP, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_EUCJP, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO885915, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_MACINTOSH, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO88592, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_WINDOWS1250, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO88595, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_WINDOWS1251, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_KOI8R, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_KOI8U, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO88597, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_WINDOWS1253, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO88594, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO885913, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_WINDOWS1257, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO88593, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO885910, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO885914, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO885916, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_WINDOWS1254, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO88596, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_WINDOWS1256, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO88598, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_ISO88598I, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_WINDOWS1255, true);
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_WINDOWS1258, true);

    //// Zoom
    //command_updater_.UpdateCommandEnabled(IDC_ZOOM_MENU, true);
    //command_updater_.UpdateCommandEnabled(IDC_ZOOM_PLUS, true);
    //command_updater_.UpdateCommandEnabled(IDC_ZOOM_NORMAL, true);
    //command_updater_.UpdateCommandEnabled(IDC_ZOOM_MINUS, true);

    //// Show various bits of UI
    //UpdateOpenFileState();
    //command_updater_.UpdateCommandEnabled(IDC_CREATE_SHORTCUTS, false);
    //UpdateCommandsForDevTools();
    //command_updater_.UpdateCommandEnabled(IDC_TASK_MANAGER, true);
    //command_updater_.UpdateCommandEnabled(IDC_SHOW_HISTORY, true);
    //command_updater_.UpdateCommandEnabled(IDC_SHOW_BOOKMARK_MANAGER,
    //    browser_defaults::bookmarks_enabled);
    //command_updater_.UpdateCommandEnabled(IDC_SHOW_DOWNLOADS, true);
    //command_updater_.UpdateCommandEnabled(IDC_HELP_PAGE, true);
    //command_updater_.UpdateCommandEnabled(IDC_IMPORT_SETTINGS, true);
    //command_updater_.UpdateCommandEnabled(IDC_BOOKMARKS_MENU, true);

    //command_updater_.UpdateCommandEnabled(
    //    IDC_SHOW_SYNC_SETUP, profile_->GetOriginalProfile()->IsSyncAccessible());

    //ExtensionService* extension_service = profile()->GetExtensionService();
    //bool enable_extensions =
    //    extension_service && extension_service->extensions_enabled();
    //command_updater_.UpdateCommandEnabled(IDC_MANAGE_EXTENSIONS,
    //    enable_extensions);

    //// Initialize other commands based on the window type.
    //bool normal_window = is_type_tabbed();

    //// Navigation commands
    //command_updater_.UpdateCommandEnabled(IDC_HOME, normal_window);

    //// Window management commands
    //// TODO(rohitrao): Disable fullscreen on non-Lion?
    //command_updater_.UpdateCommandEnabled(IDC_FULLSCREEN,
    //    !(is_type_panel() && is_app()));
    //command_updater_.UpdateCommandEnabled(IDC_SELECT_NEXT_TAB, normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_SELECT_PREVIOUS_TAB,
    //    normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_MOVE_TAB_NEXT, normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_MOVE_TAB_PREVIOUS, normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_0, normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_1, normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_2, normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_3, normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_4, normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_5, normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_6, normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_7, normal_window);
    //command_updater_.UpdateCommandEnabled(IDC_SELECT_LAST_TAB, normal_window);

    //// Clipboard commands
    //command_updater_.UpdateCommandEnabled(IDC_COPY_URL, !is_devtools());

    //// Find-in-page
    //command_updater_.UpdateCommandEnabled(IDC_FIND, !is_devtools());
    //command_updater_.UpdateCommandEnabled(IDC_FIND_NEXT, !is_devtools());
    //command_updater_.UpdateCommandEnabled(IDC_FIND_PREVIOUS, !is_devtools());

    //// Show various bits of UI
    //command_updater_.UpdateCommandEnabled(IDC_CLEAR_BROWSING_DATA, normal_window);

    //// The upgrade entry and the view incompatibility entry should always be
    //// enabled. Whether they are visible is a separate matter determined on menu
    //// show.
    //command_updater_.UpdateCommandEnabled(IDC_UPGRADE_DIALOG, true);
    //command_updater_.UpdateCommandEnabled(IDC_VIEW_INCOMPATIBILITIES, true);

    //// View Background Pages entry is always enabled, but is hidden if there are
    //// no background pages.
    //command_updater_.UpdateCommandEnabled(IDC_VIEW_BACKGROUND_PAGES, true);

    //// Initialize other commands whose state changes based on fullscreen mode.
    //UpdateCommandsForFullscreenMode(false);

    //UpdateCommandsForContentRestrictionState();

    //UpdateCommandsForBookmarkEditing();
}

void Browser::UpdateCommandsForFullscreenMode(bool is_fullscreen)
{
    const bool show_main_ui = is_type_tabbed() && !is_fullscreen;

    bool main_not_fullscreen = show_main_ui && !is_fullscreen;

    // Navigation commands
    //command_updater_.UpdateCommandEnabled(IDC_OPEN_CURRENT_URL, show_main_ui);

    //// Window management commands
    //command_updater_.UpdateCommandEnabled(IDC_SHOW_AS_TAB,
    //    type_ != TYPE_TABBED && !is_fullscreen);

    //// Focus various bits of UI
    //command_updater_.UpdateCommandEnabled(IDC_FOCUS_TOOLBAR, show_main_ui);
    //command_updater_.UpdateCommandEnabled(IDC_FOCUS_LOCATION, show_main_ui);
    //command_updater_.UpdateCommandEnabled(IDC_FOCUS_SEARCH, show_main_ui);
    //command_updater_.UpdateCommandEnabled(
    //    IDC_FOCUS_MENU_BAR, main_not_fullscreen);
    //command_updater_.UpdateCommandEnabled(
    //    IDC_FOCUS_NEXT_PANE, main_not_fullscreen);
    //command_updater_.UpdateCommandEnabled(
    //    IDC_FOCUS_PREVIOUS_PANE, main_not_fullscreen);
    //command_updater_.UpdateCommandEnabled(
    //    IDC_FOCUS_BOOKMARKS, main_not_fullscreen);
    //command_updater_.UpdateCommandEnabled(
    //    IDC_FOCUS_CHROMEOS_STATUS, main_not_fullscreen);

    //// Show various bits of UI
    //command_updater_.UpdateCommandEnabled(IDC_DEVELOPER_MENU, show_main_ui);
    //command_updater_.UpdateCommandEnabled(IDC_FEEDBACK, show_main_ui);
    //command_updater_.UpdateCommandEnabled(IDC_IMPORT_SETTINGS, show_main_ui);
    //command_updater_.UpdateCommandEnabled(IDC_SYNC_BOOKMARKS,
    //    show_main_ui && profile_->GetOriginalProfile()->IsSyncAccessible());

    //command_updater_.UpdateCommandEnabled(IDC_OPTIONS, show_main_ui);
    //command_updater_.UpdateCommandEnabled(IDC_EDIT_SEARCH_ENGINES, show_main_ui);
    //command_updater_.UpdateCommandEnabled(IDC_VIEW_PASSWORDS, show_main_ui);
    //command_updater_.UpdateCommandEnabled(IDC_ABOUT, show_main_ui);
    //command_updater_.UpdateCommandEnabled(IDC_SHOW_APP_MENU, show_main_ui);
    //command_updater_.UpdateCommandEnabled(IDC_TOGGLE_VERTICAL_TABS, show_main_ui);
    //command_updater_.UpdateCommandEnabled(IDC_COMPACT_NAVBAR, show_main_ui);

    UpdateCommandsForBookmarkBar();
}

void Browser::UpdateCommandsForTabState()
{
    TabContents* current_tab = GetSelectedTabContents();
    TabContentsWrapper* current_tab_wrapper = GetSelectedTabContentsWrapper();
    if(!current_tab || !current_tab_wrapper) // May be NULL during tab restore.
    {
        return;
    }

    // Window management commands
    //command_updater_.UpdateCommandEnabled(IDC_DUPLICATE_TAB,
    //    !is_app() && CanDuplicateContentsAt(active_index()));

    //// Page-related commands
    //window_->SetStarredState(
    //    current_tab_wrapper->bookmark_tab_helper()->is_starred());
    //command_updater_.UpdateCommandEnabled(IDC_VIEW_SOURCE,
    //    current_tab->controller().CanViewSource());
    //command_updater_.UpdateCommandEnabled(IDC_EMAIL_PAGE_LOCATION,
    //    current_tab->ShouldDisplayURL() && current_tab->GetURL().is_valid());
    //if(is_devtools())
    //{
    //    command_updater_.UpdateCommandEnabled(IDC_OPEN_FILE, false);
    //}

    //// Changing the encoding is not possible on Chrome-internal webpages.
    //bool is_chrome_internal = HasInternalURL(nc.GetActiveEntry());
    //command_updater_.UpdateCommandEnabled(IDC_ENCODING_MENU,
    //    !is_chrome_internal && SavePackage::IsSavableContents(
    //    current_tab->contents_mime_type()));

    //// Show various bits of UI
    //// TODO(pinkerton): Disable app-mode in the model until we implement it
    //// on the Mac. Be sure to remove both ifdefs. http://crbug.com/13148
    //command_updater_.UpdateCommandEnabled(IDC_CREATE_SHORTCUTS,
    //    web_app::IsValidUrl(current_tab->GetURL()));

    UpdateCommandsForContentRestrictionState();
    UpdateCommandsForBookmarkEditing();
}

void Browser::UpdateCommandsForContentRestrictionState()
{
    //int restrictions = GetContentRestrictionsForSelectedTab();

    //command_updater_.UpdateCommandEnabled(
    //    IDC_COPY, !(restrictions & CONTENT_RESTRICTION_COPY));
    //command_updater_.UpdateCommandEnabled(
    //    IDC_CUT, !(restrictions & CONTENT_RESTRICTION_CUT));
    //command_updater_.UpdateCommandEnabled(
    //    IDC_PASTE, !(restrictions & CONTENT_RESTRICTION_PASTE));
    //UpdateSaveAsState(restrictions);
    //UpdatePrintingState(restrictions);
}

void Browser::UpdateReloadStopState(bool is_loading, bool force)
{
    window_->UpdateReloadStopState(is_loading, force);
    //command_updater_.UpdateCommandEnabled(IDC_STOP, is_loading);
}

void Browser::UpdateCommandsForBookmarkEditing()
{
    //bool enabled =
    //    profile_->GetPrefs()->GetBoolean(prefs::kEditBookmarksEnabled) &&
    //    browser_defaults::bookmarks_enabled;

    //command_updater_.UpdateCommandEnabled(IDC_BOOKMARK_PAGE,
    //    enabled && is_type_tabbed());
    //command_updater_.UpdateCommandEnabled(IDC_BOOKMARK_ALL_TABS,
    //    enabled && CanBookmarkAllTabs());
}

void Browser::UpdateCommandsForBookmarkBar()
{
    const bool show_main_ui = is_type_tabbed() &&
        (!window_ || !window_->IsFullscreen());
    //command_updater_.UpdateCommandEnabled(IDC_SHOW_BOOKMARK_BAR,
    //    browser_defaults::bookmarks_enabled &&
    //    !profile_->GetPrefs()->IsManagedPreference(prefs::kEnableBookmarkBar) &&
    //    show_main_ui);
}

void Browser::UpdateSaveAsState(int content_restrictions)
{
    //bool enabled = !(content_restrictions & CONTENT_RESTRICTION_SAVE);
    //PrefService* state = g_browser_process->local_state();
    //if(state)
    //{
    //    enabled = enabled && state->GetBoolean(prefs::kAllowFileSelectionDialogs);
    //}

    //command_updater_.UpdateCommandEnabled(IDC_SAVE_PAGE, enabled);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, UI update coalescing and handling (private):

void Browser::UpdateToolbar(bool should_restore_state)
{
    window_->UpdateToolbar(GetSelectedTabContentsWrapper(), should_restore_state);
}

void Browser::ScheduleUIUpdate(const TabContents* source,
                               unsigned changed_flags)
{
    if(!source)
    {
        return;
    }

    // Do some synchronous updates.
    if(changed_flags & TabContents::INVALIDATE_URL &&
        source==GetSelectedTabContents())
    {
            // Only update the URL for the current tab. Note that we do not update
            // the navigation commands since those would have already been updated
            // synchronously by NavigationStateChanged.
            UpdateToolbar(false);
            changed_flags &= ~TabContents::INVALIDATE_URL;
    }
    if(changed_flags & TabContents::INVALIDATE_LOAD)
    {
        // Update the loading state synchronously. This is so the throbber will
        // immediately start/stop, which gives a more snappy feel. We want to do
        // this for any tab so they start & stop quickly.
        //tab_handler_->GetTabStripModel()->UpdateTabContentsStateAt(
        //    tab_handler_->GetTabStripModel()->GetIndexOfController(
        //    &source->controller()),
        //    TabStripModelObserver::LOADING_ONLY);
        // The status bubble needs to be updated during INVALIDATE_LOAD too, but
        // we do that asynchronously by not stripping INVALIDATE_LOAD from
        // changed_flags.
    }

    if(changed_flags & TabContents::INVALIDATE_TITLE && !source->IsLoading())
    {
        // To correctly calculate whether the title changed while not loading
        // we need to process the update synchronously. This state only matters for
        // the TabStripModel, so we notify the TabStripModel now and notify others
        // asynchronously.
        //tab_handler_->GetTabStripModel()->UpdateTabContentsStateAt(
        //    tab_handler_->GetTabStripModel()->GetIndexOfController(
        //    &source->controller()),
        //    TabStripModelObserver::TITLE_NOT_LOADING);
    }

    // If the only updates were synchronously handled above, we're done.
    if(changed_flags == 0)
    {
        return;
    }

    // Save the dirty bits.
    scheduled_updates_[source] |= changed_flags;

    if(chrome_updater_factory_.empty())
    {
        // No task currently scheduled, start another.
        MessageLoop::current()->PostDelayedTask(
            chrome_updater_factory_.NewRunnableMethod(
            &Browser::ProcessPendingUIUpdates),
            kUIUpdateCoalescingTimeMS);
    }
}

void Browser::ProcessPendingUIUpdates()
{
#ifndef NDEBUG
    // Validate that all tabs we have pending updates for exist. This is scary
    // because the pending list must be kept in sync with any detached or
    // deleted tabs.
    for(UpdateMap::const_iterator i=scheduled_updates_.begin();
        i!=scheduled_updates_.end(); ++i)
    {
        bool found = false;
        for(int tab=0; tab<tab_count(); ++tab)
        {
            if(GetTabContentsAt(tab) == i->first)
            {
                found = true;
                break;
            }
        }
        DCHECK(found);
    }
#endif

    chrome_updater_factory_.RevokeAll();

    for(UpdateMap::const_iterator i=scheduled_updates_.begin();
        i!=scheduled_updates_.end(); ++i)
    {
        // Do not dereference |contents|, it may be out-of-date!
        const TabContents* contents = i->first;
        unsigned flags = i->second;

        if(contents == GetSelectedTabContents())
        {
            // Updates that only matter when the tab is selected go here.

            if(flags & TabContents::INVALIDATE_PAGE_ACTIONS)
            {
                LocationBar* location_bar = window()->GetLocationBar();
                if(location_bar)
                {
                    location_bar->UpdatePageActions();
                }
            }
            // Updating the URL happens synchronously in ScheduleUIUpdate.
            //if(flags & TabContents::INVALIDATE_LOAD && GetStatusBubble())
            //{
            //    GetStatusBubble()->SetStatus(
            //        GetSelectedTabContentsWrapper()->GetStatusText());
            //}

            if(flags & (TabContents::INVALIDATE_TAB |
                TabContents::INVALIDATE_TITLE))
            {
                // TODO(pinkerton): Disable app-mode in the model until we implement it
                // on the Mac. Be sure to remove both ifdefs. http://crbug.com/13148
                //command_updater_.UpdateCommandEnabled(IDC_CREATE_SHORTCUTS,
                //    web_app::IsValidUrl(contents->GetURL()));

                window_->UpdateTitleBar();
            }
        }

        // Updates that don't depend upon the selected state go here.
        if(flags & (TabContents::INVALIDATE_TAB | TabContents::INVALIDATE_TITLE))
        {
            tab_handler_->GetTabStripModel()->UpdateTabContentsStateAt(
                tab_handler_->GetTabStripModel()->GetWrapperIndex(contents),
                TabStripModelObserver::ALL);
        }

        // We don't need to process INVALIDATE_STATE, since that's not visible.
    }

    scheduled_updates_.clear();
}

void Browser::RemoveScheduledUpdatesFor(TabContents* contents)
{
    if(!contents)
    {
        return;
    }

    UpdateMap::iterator i = scheduled_updates_.find(contents);
    if(i != scheduled_updates_.end())
    {
        scheduled_updates_.erase(i);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Browser, OnBeforeUnload handling (private):

void Browser::ProcessPendingTabs()
{
    if(!is_attempting_to_close_browser_)
    {
        // Because we might invoke this after a delay it's possible for the value of
        // is_attempting_to_close_browser_ to have changed since we scheduled the
        // task.
        return;
    }

    if(HasCompletedUnloadProcessing())
    {
        // We've finished all the unload events and can proceed to close the
        // browser.
        OnWindowClosing();
        return;
    }

    // Process beforeunload tabs first. When that queue is empty, process
    // unload tabs.
    if(!tabs_needing_before_unload_fired_.empty())
    {
        TabContents* tab = *(tabs_needing_before_unload_fired_.begin());
        // Null check render_view_host here as this gets called on a PostTask and
        // the tab's render_view_host may have been nulled out.
        //if(tab->render_view_host())
        //{
        //    tab->render_view_host()->FirePageBeforeUnload(false);
        //}
        //else
        {
            ClearUnloadState(tab, true);
        }
    }
    else if(!tabs_needing_unload_fired_.empty())
    {
        // We've finished firing all beforeunload events and can proceed with unload
        // events.
        // TODO(ojan): We should add a call to browser_shutdown::OnShutdownStarting
        // somewhere around here so that we have accurate measurements of shutdown
        // time.
        // TODO(ojan): We can probably fire all the unload events in parallel and
        // get a perf benefit from that in the cases where the tab hangs in it's
        // unload handler or takes a long time to page in.
        TabContents* tab = *(tabs_needing_unload_fired_.begin());
        // Null check render_view_host here as this gets called on a PostTask and
        // the tab's render_view_host may have been nulled out.
        //if(tab->render_view_host())
        //{
        //    tab->render_view_host()->ClosePage();
        //}
        //else
        {
            ClearUnloadState(tab, true);
        }
    }
    else
    {
        NOTREACHED();
    }
}

bool Browser::HasCompletedUnloadProcessing() const
{
    return is_attempting_to_close_browser_ &&
        tabs_needing_before_unload_fired_.empty() &&
        tabs_needing_unload_fired_.empty();
}

void Browser::CancelWindowClose()
{
    // Closing of window can be canceled from:
    // - canceling beforeunload
    // - disallowing closing from IsClosingPermitted.
    DCHECK(is_attempting_to_close_browser_);
    tabs_needing_before_unload_fired_.clear();
    tabs_needing_unload_fired_.clear();
    is_attempting_to_close_browser_ = false;

    // Inform TabCloseableStateWatcher that closing of window has been canceled.
    //TabCloseableStateWatcher* watcher =
    //    g_browser_process->tab_closeable_state_watcher();
    //if(watcher)
    //{
    //    watcher->OnWindowCloseCanceled(this);
    //}
}

bool Browser::RemoveFromSet(UnloadListenerSet* set, TabContents* tab)
{
    DCHECK(is_attempting_to_close_browser_);

    UnloadListenerSet::iterator iter = std::find(set->begin(), set->end(), tab);
    if(iter != set->end())
    {
        set->erase(iter);
        return true;
    }
    return false;
}

void Browser::ClearUnloadState(TabContents* tab, bool process_now)
{
    // Closing of browser could be canceled (via IsClosingPermitted) between the
    // time when request was initiated and when this method is called, so check
    // for is_attempting_to_close_browser_ flag before proceeding.
    if(is_attempting_to_close_browser_)
    {
        RemoveFromSet(&tabs_needing_before_unload_fired_, tab);
        RemoveFromSet(&tabs_needing_unload_fired_, tab);
        if(process_now)
        {
            ProcessPendingTabs();
        }
        else
        {
            MessageLoop::current()->PostTask(
                method_factory_.NewRunnableMethod(&Browser::ProcessPendingTabs));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Browser, Assorted utility functions (private):

// static
Browser* Browser::GetTabbedBrowser()
{
    return BrowserList::FindTabbedBrowser(NULL, false/*profile, match_incognito*/);
}

// static
Browser* Browser::GetOrCreateTabbedBrowser()
{
    Browser* browser = GetTabbedBrowser();
    if(!browser)
    {
        browser = Browser::Create();
    }
    return browser;
}

void Browser::SetAsDelegate(TabContentsWrapper* tab, Browser* delegate)
{
    // TabContents...
    tab->tab_contents()->set_delegate(delegate);
    tab->set_delegate(delegate);

    // ...and all the helpers.
    //tab->blocked_content_tab_helper()->set_delegate(delegate);
    //tab->bookmark_tab_helper()->set_delegate(delegate);
    //tab->search_engine_tab_helper()->set_delegate(delegate);
}

void Browser::CloseFrame()
{
    window_->Close();
}

void Browser::TabDetachedAtImpl(TabContentsWrapper* contents, int index,
                                DetachType type)
{
    if(type == DETACH_TYPE_DETACH)
    {
        // Save the current location bar state, but only if the tab being detached
        // is the selected tab.  Because saving state can conditionally revert the
        // location bar, saving the current tab's location bar state to a
        // non-selected tab can corrupt both tabs.
        if(contents == GetSelectedTabContentsWrapper())
        {
            LocationBar* location_bar = window()->GetLocationBar();
            if(location_bar)
            {
                location_bar->SaveStateToContents(contents->tab_contents());
            }
        }

        if(!tab_handler_->GetTabStripModel()->closing_all())
        {
            //SyncHistoryWithTabs(0);
        }
    }

    SetAsDelegate(contents, NULL);
    RemoveScheduledUpdatesFor(contents->tab_contents());

    //if(find_bar_controller_.get() &&
    //    index==tab_handler_->GetTabStripModel()->active_index())
    //{
    //    find_bar_controller_->ChangeTabContents(NULL);
    //}

    if(is_attempting_to_close_browser_)
    {
        // If this is the last tab with unload handlers, then ProcessPendingTabs
        // would call back into the TabStripModel (which is invoking this method on
        // us). Avoid that by passing in false so that the call to
        // ProcessPendingTabs is delayed.
        ClearUnloadState(contents->tab_contents(), false);
    }

    //registrar_.Remove(this, content::NOTIFICATION_INTERSTITIAL_ATTACHED,
    //    Source<TabContents>(contents->tab_contents()));
    //registrar_.Remove(this, content::NOTIFICATION_TAB_CONTENTS_DISCONNECTED,
    //    Source<TabContents>(contents->tab_contents()));
}

// Centralized method for creating a TabContents, configuring and installing
// all its supporting objects and observers.
int do_config();
TabContentsWrapper* Browser::TabContentsFactory(
    int routing_id,
    const TabContents* base_tab_contents,
	bool isDuplicateSourceContent)
{
	if (isDuplicateSourceContent && NULL != base_tab_contents){
		base_tab_contents->dupCfg2Global();
	}else{
		if (!do_config()){
			return NULL;
		}
	}

    TabContents* new_contents = new TabContents(routing_id, base_tab_contents);
    TabContentsWrapper* wrapper = new TabContentsWrapper(new_contents);
    return wrapper;
}

void Browser::UpdateBookmarkBarState(BookmarkBarStateChangeReason reason)
{
    BookmarkBar::State state;
    // The bookmark bar is hidden in fullscreen mode, unless on the new tab page.
    //if((profile_->GetPrefs()->GetBoolean(prefs::kShowBookmarkBar) &&
    //    profile_->GetPrefs()->GetBoolean(prefs::kEnableBookmarkBar)) &&
    //    (!window_ || !window_->IsFullscreen()))
    {
        state = BookmarkBar::SHOW;
    }
    //else
    //{
    //    TabContentsWrapper* tab = GetSelectedTabContentsWrapper();
    //    if(tab && tab->bookmark_tab_helper()->ShouldShowBookmarkBar())
    //    {
    //        state = BookmarkBar::DETACHED;
    //    }
    //    else
    //    {
    //        state = BookmarkBar::HIDDEN;
    //    }
    //}
    if(state == bookmark_bar_state_)
    {
        return;
    }

    bookmark_bar_state_ = state;

    if(!window_)
    {
        return; // This is called from the constructor when window_ is NULL.
    }

    if(reason == BOOKMARK_BAR_STATE_CHANGE_TAB_SWITCH)
    {
        // Don't notify BrowserWindow on a tab switch as at the time this is invoked
        // BrowserWindow hasn't yet switched tabs. The BrowserWindow implementations
        // end up querying state once they process the tab switch.
        return;
    }

    BookmarkBar::AnimateChangeType animate_type =
        (reason == BOOKMARK_BAR_STATE_CHANGE_PREF_CHANGE) ?
        BookmarkBar::ANIMATE_STATE_CHANGE :
    BookmarkBar::DONT_ANIMATE_STATE_CHANGE;
    window_->BookmarkBarStateChanged(animate_type);
}