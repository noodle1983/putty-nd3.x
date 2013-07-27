
#ifndef __browser_h__
#define __browser_h__

#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/basic_types.h"
#include "base/memory/scoped_ptr.h"
#include "base/task.h"

#include "ui_gfx/rect.h"

#include "ui_base/ui_base_types.h"

#include "bookmark_bar.h"
#include "bookmark_tab_helper_delegate.h"
#include "dock_info.h"
#include "page_navigator.h"
#include "session_id.h"
#include "tab_contents_delegate.h"
#include "tab_contents_wrapper_delegate.h"
#include "tab_handler.h"
#include "toolbar_model.h"

class SkBitmap;

class BrowserWindow;

class Browser : public TabHandlerDelegate,
    public TabContentsDelegate,
    public TabContentsWrapperDelegate,
    public BookmarkTabHelperDelegate,
    public PageNavigator
{
public:
    // SessionService::WindowType mirrors these values.  If you add to this
    // enum, look at SessionService::WindowType to see if it needs to be
    // updated.
    enum Type
    {
        // If you add a new type, consider updating the test
        // BrowserTest.StartMaximized.
        TYPE_TABBED = 1,
        TYPE_POPUP = 2,
        TYPE_PANEL = 3,
    };

    // Possible elements of the Browser window.
    enum WindowFeature
    {
        FEATURE_NONE = 0,
        FEATURE_TITLEBAR = 1,
        FEATURE_TABSTRIP = 2,
        FEATURE_TOOLBAR = 4,
        FEATURE_LOCATIONBAR = 8,
        FEATURE_BOOKMARKBAR = 16,
        FEATURE_INFOBAR = 32,
        FEATURE_SIDEBAR = 64,
    };

    struct CreateParams
    {
        CreateParams(Type type);

        // The browser type.
        Type type;

        // The bounds of the window to open.
        gfx::Rect initial_bounds;
    };

    // Constructors, Creation, Showing //////////////////////////////////////////

    // Creates a new browser of the given |type| and for the given |profile|. The
    // Browser has a NULL window after its construction, InitBrowserWindow must
    // be called after configuration for window() to be valid.
    // Avoid using this constructor directly if you can use one of the Create*()
    // methods below. This applies to almost all non-testing code.
    Browser(Type type);
    virtual ~Browser();

    // Creates a normal tabbed browser with the specified profile. The Browser's
    // window is created by this function call.
    static Browser* Create();

    // Like Create, but creates a browser of the specified parameters.
    static Browser* CreateWithParams(const CreateParams& params);

    // Like Create, but creates a browser of the specified type.
    static Browser* CreateForType(Type type);

    // Set overrides for the initial window bounds and maximized state.
    void set_override_bounds(const gfx::Rect& bounds)
    {
        override_bounds_ = bounds;
    }
    void set_show_state(ui::WindowShowState show_state)
    {
        show_state_ = show_state;
    }
    // Return true if the initial window bounds have been overridden.
    bool bounds_overridden() const
    {
        return !override_bounds_.IsEmpty();
    }

    // Set indicator that this browser is being created via session restore.
    // This is used on the Mac (only) to determine animation style when the
    // browser window is shown.
    void set_is_session_restore(bool is_session_restore)
    {
        is_session_restore_ = is_session_restore;
    }
    bool is_session_restore() const
    {
        return is_session_restore_;
    }

    // Creates the Browser Window. Prefer to use the static helpers above where
    // possible. This does not show the window. You need to call window()->Show()
    // to show it.
    void InitBrowserWindow();

    // Accessors ////////////////////////////////////////////////////////////////

    Type type() const { return type_; }
    gfx::Rect override_bounds() const { return override_bounds_; }

    // |window()| will return NULL if called before |CreateBrowserWindow()|
    // is done.
    BrowserWindow* window() const { return window_; }
    ToolbarModel* toolbar_model() { return &toolbar_model_; }
    const SessionID& session_id() const { return session_id_; }

    // Returns the state of the bookmark bar.
    BookmarkBar::State bookmark_bar_state() const { return bookmark_bar_state_; }

    // Browser Creation Helpers /////////////////////////////////////////////////

    // Opens a new window with the default blank tab.
    static void OpenEmptyWindow();

    // Opens the specified URL in a new browser window in an incognito session.
    // If there is already an existing active incognito session for the specified
    // |profile|, that session is re-used.
    static void OpenURLOffTheRecord(const Url& url);

    // Opens a new window and opens the bookmark manager.
    static void OpenBookmarkManagerWindow();

    // State Storage and Retrieval for UI ///////////////////////////////////////

    // Save and restore the window position.
    std::string GetWindowPlacementKey() const;
    bool ShouldSaveWindowPlacement() const;
    void SaveWindowPlacement(const gfx::Rect& bounds,
        ui::WindowShowState show_state);
    gfx::Rect GetSavedWindowBounds() const;
    ui::WindowShowState GetSavedWindowShowState() const;

    // Gets the Favicon of the page in the selected tab.
    SkBitmap GetCurrentPageIcon() const;

    // Gets the title of the window based on the selected tab's title.
    string16 GetWindowTitleForCurrentTab() const;

    // Prepares a title string for display (removes embedded newlines, etc).
    static void FormatTitleForDisplay(string16* title);

    // OnBeforeUnload handling //////////////////////////////////////////////////

    // Gives beforeunload handlers the chance to cancel the close.
    bool ShouldCloseWindow();

    bool IsAttemptingToCloseBrowser() const
    {
        return is_attempting_to_close_browser_;
    }

    // Invoked when the window containing us is closing. Performs the necessary
    // cleanup.
    void OnWindowClosing();

    // OnWindowActivationChanged handling ///////////////////////////////////////

    // Invoked when the window containing us is activated.
    void OnWindowActivated();

    // TabStripModel pass-thrus /////////////////////////////////////////////////

    TabStripModel* tabstrip_model() const
    {
        // TODO(beng): remove this accessor. It violates google style.
        return tab_handler_->GetTabStripModel();
    }

    int tab_count() const;
    int active_index() const;
    int GetIndexOfController(const NavigationController* controller) const;

    // TODO(dpapad): Rename to GetActiveTabContentsWrapper().
    TabContentsWrapper* GetSelectedTabContentsWrapper() const;
    TabContentsWrapper* GetTabContentsWrapperAt(int index) const;
    // Same as above but correctly handles if GetSelectedTabContents() is NULL
    // in the model before dereferencing to get the raw TabContents.
    // TODO(pinkerton): These should really be returning TabContentsWrapper
    // objects, but that would require changing about 50+ other files. In order
    // to keep changes localized, the default is to return a TabContents. Note
    // this differs from the TabStripModel because it has far fewer clients.
    // TODO(dpapad): Rename to GetActiveTabContents().
    TabContents* GetSelectedTabContents() const;
    TabContents* GetTabContentsAt(int index) const;
    void ActivateTabAt(int index, bool user_gesture);
    void CloseAllTabs();

    // Tab adding/showing functions /////////////////////////////////////////////

    // Returns the index to insert a tab at during session restore and startup.
    // |relative_index| gives the index of the url into the number of tabs that
    // are going to be opened. For example, if three urls are passed in on the
    // command line this is invoked three times with the values 0, 1 and 2.
    int GetIndexForInsertionDuringRestore(int relative_index);

    // Adds a selected tab with the specified URL and transition, returns the
    // created TabContents.
    TabContentsWrapper* AddSelectedTabWithURL(const Url& url);

    // Add a new tab, given a TabContents. A TabContents appropriate to
    // display the last committed entry is created and returned.
    TabContents* AddTab(TabContentsWrapper* tab_contents);

    // Creates a new tab with the already-created TabContents 'new_contents'.
    // The window for the added contents will be reparented correctly when this
    // method returns.  If |disposition| is NEW_POPUP, |pos| should hold the
    // initial position.
    void AddTabContents(TabContents* new_contents,
        WindowOpenDisposition disposition,
        const gfx::Rect& initial_pos,
        bool user_gesture);
    void CloseTabContents(TabContents* contents);

    // Called when a popup select is about to be displayed.
    void BrowserRenderWidgetShowing();

    // Notification that the bookmark bar has changed size.  We need to resize the
    // content area and notify our InfoBarContainer.
    void BookmarkBarSizeChanged(bool is_animating);

    // Navigate to an index in the tab history, opening a new tab depending on the
    // disposition.
    bool NavigateToIndexWithDisposition(int index, WindowOpenDisposition disp);

    // Show a given a URL. If a tab with the same URL (ignoring the ref) is
    // already visible in this browser, it becomes selected. Otherwise a new tab
    // is created.
    void ShowSingletonTab(const Url& url);

    // Same as ShowSingletonTab, but does not ignore ref.
    void ShowSingletonTabRespectRef(const Url& url);

    // Invoked when the fullscreen state of the window changes.
    // BrowserWindow::SetFullscreen invokes this after the window has become
    // fullscreen.
    void WindowFullscreenStateChanged();

    // Sends a notification that the fullscreen state has changed.
    void NotifyFullscreenChange();

    // Assorted browser commands ////////////////////////////////////////////////

    // NOTE: Within each of the following sections, the IDs are ordered roughly by
    // how they appear in the GUI/menus (left to right, top to bottom, etc.).

    // Navigation commands
    bool CanGoBack() const;
    void GoBack(WindowOpenDisposition disposition);
    bool CanGoForward() const;
    void GoForward(WindowOpenDisposition disposition);
    void Reload(WindowOpenDisposition disposition);
    void ReloadIgnoringCache(WindowOpenDisposition disposition);  // Shift-reload.
    void Home(WindowOpenDisposition disposition);
    void OpenCurrentURL();
    void Stop();
    // Window management commands
    void NewWindow();
    void CloseWindow();
    void NewTab();
    void CloseTab();
    void SelectNextTab();
    void SelectPreviousTab();
    void OpenTabpose();
    void MoveTabNext();
    void MoveTabPrevious();
    void SelectNumberedTab(int index);
    void SelectLastTab();
    void DuplicateTab();
    void WriteCurrentURLToClipboard();
    void ConvertPopupToTabbedBrowser();
    // In kiosk mode, the first toggle is valid, the rest is discarded.
    void ToggleFullscreenMode();
    void Exit();

    // Page-related commands
    void BookmarkCurrentPage();
    void SavePage();

    // Returns true if the Browser supports the specified feature. The value of
    // this varies during the lifetime of the browser. For example, if the window
    // is fullscreen this may return a different value. If you only care about
    // whether or not it's possible for the browser to support a particular
    // feature use |CanSupportWindowFeature|.
    bool SupportsWindowFeature(WindowFeature feature) const;

    // Returns true if the Browser can support the specified feature. See comment
    // in |SupportsWindowFeature| for details on this.
    bool CanSupportWindowFeature(WindowFeature feature) const;

    // Clipboard commands
    void Cut();
    void Copy();
    void Paste();

    // Find-in-page
    void Find();
    void FindNext();
    void FindPrevious();

    // Focus various bits of UI
    void FocusToolbar();
    void FocusLocationBar(); // Also selects any existing text.
    void FocusSearch();
    void FocusAppMenu();
    void FocusBookmarksToolbar();
    void FocusNextPane();
    void FocusPreviousPane();

    // Show various bits of UI
    void OpenTaskManager(bool highlight_background_resources);
    void OpenBugReportDialog();

    void ToggleBookmarkBar();

    void OpenBookmarkManager();
    void OpenBookmarkManagerForNode(int64 node_id);
    void OpenBookmarkManagerEditNode(int64 node_id);
    void OpenBookmarkManagerAddNodeIn(int64 node_id);
    void ShowAppMenu();
    void ShowHistoryTab();
    void ShowAboutConflictsTab();
    void ShowBrokenPageTab(TabContents* contents);
    void ShowOptionsTab(const std::string& sub_page);
    void OpenClearBrowsingDataDialog();
    void OpenOptionsDialog();
    void OpenPasswordManager();
    void OpenImportSettingsDialog();
    void OpenAboutChromeDialog();
    void OpenUpdateChromeDialog();
    void ShowHelpTab();
    void OpenAutofillHelpTabAndActivate();
    void OpenPrivacyDashboardTabAndActivate();
    void OpenSearchEngineOptionsDialog();
    void OpenPluginsTabAndActivate();

    /////////////////////////////////////////////////////////////////////////////

    // Helper function to run unload listeners on a TabContents.
    static bool RunUnloadEventsHelper(TabContents* contents);

    // Returns the Browser which contains the tab with the given
    // NavigationController, also filling in |index| (if valid) with the tab's
    // index in the tab strip.
    // Returns NULL if not found.
    // This call is O(N) in the number of tabs.
    static Browser* GetBrowserForController(
        const NavigationController* controller, int* index);

    // Retrieve the last active tabbed browser.
    static Browser* GetTabbedBrowser();

    // Retrieve the last active tabbed browser.
    // Creates a new Browser if none are available.
    static Browser* GetOrCreateTabbedBrowser();

    // Calls ExecuteCommandWithDisposition with the given disposition.
    void ExecuteCommandWithDisposition(int id, WindowOpenDisposition);

    // Executes a command if it's enabled.
    // Returns true if the command is executed.
    bool ExecuteCommandIfEnabled(int id);

    // Sets if command execution shall be blocked. If |block| is true then
    // following calls to ExecuteCommand() or ExecuteCommandWithDisposition()
    // method will not execute the command, and the last blocked command will be
    // recorded for retrieval.
    void SetBlockCommandExecution(bool block);

    // Gets the last blocked command after calling SetBlockCommandExecution(true).
    // Returns the command id or -1 if there is no command blocked. The
    // disposition type of the command will be stored in |*disposition| if it's
    // not null.
    int GetLastBlockedCommand(WindowOpenDisposition* disposition);

    // Called by browser::Navigate() when a navigation has occurred in a tab in
    // this Browser. Updates the UI for the start of this navigation.
    void UpdateUIForNavigationInTab(TabContentsWrapper* contents,
        bool user_initiated);

    // Called by browser::Navigate() to retrieve the home page if no URL is
    // specified.
    Url GetHomePage() const;

    // Shows the cookies collected in the tab contents.
    void ShowCollectedCookiesDialog(TabContents* tab_contents);

    // Interface implementations ////////////////////////////////////////////////

    // Overridden from PageNavigator:
    // Deprecated. Please use the one-argument variant instead.
    // TODO(adriansc): Remove this method once refactoring changed all call sites.
    virtual TabContents* OpenURL(const Url& url,
        const Url& referrer,
        WindowOpenDisposition disposition) OVERRIDE;
    virtual TabContents* OpenURL(const OpenURLParams& params) OVERRIDE;

    // Overridden from CommandUpdater::CommandUpdaterDelegate:
    virtual void ExecuteCommand(int id);

    // Centralized method for creating a TabContents, configuring and installing
    // all its supporting objects and observers.
    static TabContentsWrapper* TabContentsFactory(
        int routing_id,
        const TabContents* base_tab_contents,
		bool isWithGlobalCfg);

    // Overridden from TabHandlerDelegate:
    virtual Browser* AsBrowser();

    // Overridden from TabStripModelDelegate:
    virtual TabContentsWrapper* AddBlankTab(bool foreground);
	virtual TabContentsWrapper* AddTabWithGlobalCfg(bool foreground);
	virtual TabContentsWrapper*  DuplicateCurrentTab();
    virtual TabContentsWrapper* AddBlankTabAt(int index, bool foreground);
    virtual Browser* CreateNewStripWithContents(
        TabContentsWrapper* detached_contents,
        const gfx::Rect& window_bounds,
        const DockInfo& dock_info,
        bool maximize);
    virtual int GetDragActions() const;
    // Construct a TabContents for a given URL, profile and transition type.
    // If instance is not null, its process will be used to render the tab.
    virtual TabContentsWrapper* CreateTabContentsForURL(const Url& url,
        const Url& referrer,
        bool defer_load) const;
    virtual bool CanDuplicateContentsAt(int index);
    virtual void DuplicateContentsAt(int index);
    virtual void CloseFrameAfterDragSession();
    virtual void CreateHistoricalTab(TabContentsWrapper* contents);
    virtual bool RunUnloadListenerBeforeClosing(TabContentsWrapper* contents);
    virtual bool CanCloseContents(std::vector<int>* indices);
    virtual bool CanBookmarkAllTabs() const;
    virtual void BookmarkAllTabs();
    virtual bool CanCloseTab() const;
    virtual bool LargeIconsPermitted() const;

    // Overridden from TabStripModelObserver:
    virtual void TabInsertedAt(TabContentsWrapper* contents,
        int index,
        bool foreground);
    virtual void TabClosingAt(TabStripModel* tab_strip_model,
        TabContentsWrapper* contents,
        int index);
    virtual void TabDetachedAt(TabContentsWrapper* contents, int index);
    virtual void TabDeactivated(TabContentsWrapper* contents);
    virtual void ActiveTabChanged(TabContentsWrapper* old_contents,
        TabContentsWrapper* new_contents,
        int index,
        bool user_gesture);
    virtual void TabMoved(TabContentsWrapper* contents,
        int from_index,
        int to_index);
    virtual void TabReplacedAt(TabStripModel* tab_strip_model,
        TabContentsWrapper* old_contents,
        TabContentsWrapper* new_contents,
        int index);
    virtual void TabStripEmpty();

    // Figure out if there are tabs that have beforeunload handlers.
    bool TabsNeedBeforeUnloadFired();

    bool is_type_tabbed() const { return type_ == TYPE_TABBED; }
    bool is_type_popup() const { return type_ == TYPE_POPUP; }
    bool is_type_panel() const { return type_ == TYPE_PANEL; }

protected:
    // Wrapper for the factory method in BrowserWindow. This allows subclasses to
    // set their own window.
    virtual BrowserWindow* CreateBrowserWindow();

private:
    // Used to describe why a tab is being detached. This is used by
    // TabDetachedAtImpl.
    enum DetachType
    {
        // Result of TabDetachedAt.
        DETACH_TYPE_DETACH,

        // Result of TabReplacedAt.
        DETACH_TYPE_REPLACE,

        // Result of the tab strip not having any significant tabs.
        DETACH_TYPE_EMPTY
    };

    // Describes where the bookmark bar state change originated from.
    enum BookmarkBarStateChangeReason
    {
        // From the constructor.
        BOOKMARK_BAR_STATE_CHANGE_INIT,

        // Change is the result of the active tab changing.
        BOOKMARK_BAR_STATE_CHANGE_TAB_SWITCH,

        // Change is the result of the bookmark bar pref changing.
        BOOKMARK_BAR_STATE_CHANGE_PREF_CHANGE,

        // Change is the result of a state change in the active tab.
        BOOKMARK_BAR_STATE_CHANGE_TAB_STATE,

        // Change is the result of window toggling in/out of fullscreen mode.
        BOOKMARK_BAR_STATE_CHANGE_TOGGLE_FULLSCREEN,
    };

    // Overridden from TabContentsDelegate:
    // Deprecated. Please use two-argument variant.
    // TODO(adriansc): Remove this method once refactoring changed all call sites.
    virtual TabContents* OpenURLFromTab(TabContents* source,
        const Url& url,
        const Url& referrer,
        WindowOpenDisposition disposition) OVERRIDE;
    virtual TabContents* OpenURLFromTab(TabContents* source,
        const OpenURLParams& params) OVERRIDE;
    virtual void NavigationStateChanged(const TabContents* source,
        unsigned changed_flags);
    virtual void AddNewContents(TabContents* source,
        TabContents* new_contents,
        WindowOpenDisposition disposition,
        const gfx::Rect& initial_pos,
        bool user_gesture);
    virtual void ActivateContents(TabContents* contents);
    virtual void DeactivateContents(TabContents* contents);
    virtual void LoadingStateChanged(TabContents* source);
    virtual void CloseContents(TabContents* source);
    virtual void MoveContents(TabContents* source, const gfx::Rect& pos);
    virtual void DetachContents(TabContents* source);
    virtual bool IsPopupOrPanel(const TabContents* source) const;
    virtual bool CanReloadContents(TabContents* source) const;
    virtual void UpdateTargetURL(TabContents* source, const Url& url) OVERRIDE;
    virtual void ContentsMouseEvent(
        TabContents* source, const gfx::Point& location, bool motion);
    virtual void ContentsZoomChange(bool zoom_in);
    virtual void SetTabContentBlocked(TabContents* contents,
        bool blocked);
    virtual void TabContentsFocused(TabContents* tab_content);
    virtual bool TakeFocus(bool reverse);
    virtual void BeforeUnloadFired(TabContents* source,
        bool proceed,
        bool* proceed_to_fire_unload);
    virtual void SetFocusToLocationBar(bool select_all);
    virtual void RenderWidgetShowing();
    virtual int GetExtraRenderViewHeight() const;
    virtual void ShowRepostFormWarningDialog(TabContents* tab_contents);
    virtual void ContentRestrictionsChanged(TabContents* source);
    virtual void RendererUnresponsive(TabContents* source);
    virtual void RendererResponsive(TabContents* source);
    virtual void WorkerCrashed(TabContents* source);
    virtual void DidNavigateMainFramePostCommit(TabContents* tab);
    virtual void DidNavigateToPendingEntry(TabContents* tab);
    virtual void ToggleFullscreenModeForTab(TabContents* tab,
        bool enter_fullscreen) OVERRIDE;

    // Overridden from TabContentsWrapperDelegate:
    //virtual void OnDidGetApplicationInfo(TabContentsWrapper* source,
    //    int32 page_id) OVERRIDE;

    // Note that the caller is responsible for deleting |old_tab_contents|.
    virtual void SwapTabContents(TabContentsWrapper* old_tab_contents,
        TabContentsWrapper* new_tab_contents);

    // Overridden from BlockedContentTabHelperDelegate:
    virtual TabContentsWrapper* GetConstrainingContentsWrapper(
        TabContentsWrapper* source);

    // Overridden from BookmarkTabHelperDelegate:
    virtual void URLStarredChanged(TabContentsWrapper* source,
        bool starred);

    // Overridden from ProfileSyncServiceObserver:
    virtual void OnStateChanged();

    // Command and state updating ///////////////////////////////////////////////

    // Initialize state for all browser commands.
    void InitCommandState();

    // Update commands whose state depends on the tab's state.
    void UpdateCommandsForTabState();

    // Updates commands when the content's restrictions change.
    void UpdateCommandsForContentRestrictionState();

    // Updates commands for bookmark editing.
    void UpdateCommandsForBookmarkEditing();

    // Updates commands that affect the bookmark bar.
    void UpdateCommandsForBookmarkBar();

    // Update commands whose state depends on whether the window is in fullscreen
    // mode.
    void UpdateCommandsForFullscreenMode(bool is_fullscreen);

    // Updates the save-page-as command state.
    void UpdateSaveAsState(int content_restrictions);

    // Ask the Reload/Stop button to change its icon, and update the Stop command
    // state.  |is_loading| is true if the current TabContents is loading.
    // |force| is true if the button should change its icon immediately.
    void UpdateReloadStopState(bool is_loading, bool force);

    // UI update coalescing and handling ////////////////////////////////////////

    // Asks the toolbar (and as such the location bar) to update its state to
    // reflect the current tab's current URL, security state, etc.
    // If |should_restore_state| is true, we're switching (back?) to this tab and
    // should restore any previous location bar state (such as user editing) as
    // well.
    void UpdateToolbar(bool should_restore_state);

    // Does one or both of the following for each bit in |changed_flags|:
    // . If the update should be processed immediately, it is.
    // . If the update should processed asynchronously (to avoid lots of ui
    //   updates), then scheduled_updates_ is updated for the |source| and update
    //   pair and a task is scheduled (assuming it isn't running already)
    //   that invokes ProcessPendingUIUpdates.
    void ScheduleUIUpdate(const TabContents* source, unsigned changed_flags);

    // Processes all pending updates to the UI that have been scheduled by
    // ScheduleUIUpdate in scheduled_updates_.
    void ProcessPendingUIUpdates();

    // Removes all entries from scheduled_updates_ whose source is contents.
    void RemoveScheduledUpdatesFor(TabContents* contents);

    // Session restore functions ////////////////////////////////////////////////

    // Tab fullscreen functions /////////////////////////////////////////////////

    // There are two different kinds of fullscreen mode - "tab fullscreen" and
    // "browser fullscreen". "Tab fullscreen" refers to when a tab enters
    // fullscreen mode via the JS fullscreen API, and "browser fullscreen"
    // refers to the user putting the browser itself into fullscreen mode from
    // the UI. The difference is that tab fullscreen has implications for how
    // the contents of the tab render (eg: a video element may grow to consume
    // the whole tab), whereas browser fullscreen mode doesn't. Therefore if a
    // user forces an exit from fullscreen, we need to notify the tab so it can
    // stop rendering in its fullscreen mode.

    // Make the current tab exit fullscreen mode if it is in it.
    void ExitTabbedFullscreenModeIfNecessary();

    // Notifies the tab that it has been forced out of fullscreen mode if
    // necessary.
    void NotifyTabOfFullscreenExitIfNecessary();

    // OnBeforeUnload handling //////////////////////////////////////////////////

    typedef std::set<TabContents*> UnloadListenerSet;

    // Processes the next tab that needs it's beforeunload/unload event fired.
    void ProcessPendingTabs();

    // Whether we've completed firing all the tabs' beforeunload/unload events.
    bool HasCompletedUnloadProcessing() const;

    // Clears all the state associated with processing tabs' beforeunload/unload
    // events since the user cancelled closing the window.
    void CancelWindowClose();

    // Removes |tab| from the passed |set|.
    // Returns whether the tab was in the set in the first place.
    // TODO(beng): this method needs a better name!
    bool RemoveFromSet(UnloadListenerSet* set, TabContents* tab);

    // Cleans up state appropriately when we are trying to close the browser and
    // the tab has finished firing its unload handler. We also use this in the
    // cases where a tab crashes or hangs even if the beforeunload/unload haven't
    // successfully fired. If |process_now| is true |ProcessPendingTabs| is
    // invoked immediately, otherwise it is invoked after a delay (PostTask).
    //
    // Typically you'll want to pass in true for |process_now|. Passing in true
    // may result in deleting |tab|. If you know that shouldn't happen (because of
    // the state of the stack), pass in false.
    void ClearUnloadState(TabContents* tab, bool process_now);

    // Assorted utility functions ///////////////////////////////////////////////

    // Sets the delegate of all the parts of the |TabContentsWrapper| that
    // are needed.
    void SetAsDelegate(TabContentsWrapper* tab, Browser* delegate);

    // Closes the frame.
    // TODO(beng): figure out if we need this now that the frame itself closes
    //             after a return to the message loop.
    void CloseFrame();

    void TabDetachedAtImpl(TabContentsWrapper* contents,
        int index, DetachType type);

    // Shared code between Reload() and ReloadIgnoringCache().
    void ReloadInternal(WindowOpenDisposition disposition, bool ignore_cache);

    // Depending on the disposition, return the current tab or a clone of the
    // current tab.
    TabContents* GetOrCloneTabForDisposition(WindowOpenDisposition disposition);

    // Sets the insertion policy of the tabstrip based on whether vertical tabs
    // are enabled.
    void UpdateTabStripModelInsertionPolicy();

    // Implementation of SupportsWindowFeature and CanSupportWindowFeature. If
    // |check_fullscreen| is true, the set of features reflect the actual state of
    // the browser, otherwise the set of features reflect the possible state of
    // the browser.
    bool SupportsWindowFeatureImpl(WindowFeature feature,
        bool check_fullscreen) const;

    // Determines if closing of browser can really be permitted after normal
    // sequence of downloads and unload handlers have given the go-ahead to close.
    // It is called from ShouldCloseWindow.  It checks with
    // TabCloseableStateWatcher to confirm if browser can really be closed.
    // Appropriate action is taken by watcher as it sees fit.
    // If watcher denies closing of browser, CancelWindowClose is called to
    // cancel closing of window.
    bool IsClosingPermitted();

    // Resets |bookmark_bar_state_| based on the active tab. Notifies the
    // BrowserWindow if necessary.
    void UpdateBookmarkBarState(BookmarkBarStateChangeReason reason);

    // Open the bookmark manager with a defined hash action.
    void OpenBookmarkManagerWithHash(const std::string& action, int64 node_id);

    // Make the current tab exit fullscreen mode. If the browser was fullscreen
    // because of that (as opposed to the user clicking the fullscreen button)
    // then take the browser out of fullscreen mode as well.
    void ExitTabbedFullscreenMode();

    // Notifies the tab that it has been forced out of fullscreen mode.
    void NotifyTabOfFullscreenExit();

    // Data members /////////////////////////////////////////////////////////////

    // This Browser's type.
    const Type type_;

    // This Browser's window.
    BrowserWindow* window_;

    // This Browser's current TabHandler.
    scoped_ptr<TabHandler> tab_handler_;

    // Unique identifier of this browser for session restore. This id is only
    // unique within the current session, and is not guaranteed to be unique
    // across sessions.
    const SessionID session_id_;

    // The model for the toolbar view.
    ToolbarModel toolbar_model_;

    // UI update coalescing and handling ////////////////////////////////////////

    typedef std::map<const TabContents*, int> UpdateMap;

    // Maps from TabContents to pending UI updates that need to be processed.
    // We don't update things like the URL or tab title right away to avoid
    // flickering and extra painting.
    // See ScheduleUIUpdate and ProcessPendingUIUpdates.
    UpdateMap scheduled_updates_;

    // The following factory is used for chrome update coalescing.
    ScopedRunnableMethodFactory<Browser> chrome_updater_factory_;

    // OnBeforeUnload handling //////////////////////////////////////////////////

    // Tracks tabs that need there beforeunload event fired before we can
    // close the browser. Only gets populated when we try to close the browser.
    UnloadListenerSet tabs_needing_before_unload_fired_;

    // Tracks tabs that need there unload event fired before we can
    // close the browser. Only gets populated when we try to close the browser.
    UnloadListenerSet tabs_needing_unload_fired_;

    // Whether we are processing the beforeunload and unload events of each tab
    // in preparation for closing the browser.
    bool is_attempting_to_close_browser_;

    /////////////////////////////////////////////////////////////////////////////

    // Override values for the bounds of the window and its maximized or minimized
    // state.
    // These are supplied by callers that don't want to use the default values.
    // The default values are typically loaded from local state (last session),
    // obtained from the last window of the same type, or obtained from the
    // shell shortcut's startup info.
    gfx::Rect override_bounds_;
    ui::WindowShowState show_state_;

    // Tracks when this browser is being created by session restore.
    bool is_session_restore_;

    // The following factory is used to close the frame at a later time.
    ScopedRunnableMethodFactory<Browser> method_factory_;

    // Indicates if command execution is blocked.
    bool block_command_execution_;

    // Stores the last blocked command id when |block_command_execution_| is true.
    int last_blocked_command_id_;

    // Stores the disposition type of the last blocked command.
    WindowOpenDisposition last_blocked_command_disposition_;

    BookmarkBar::State bookmark_bar_state_;

    // Tab to notify when the browser exits fullscreen mode.
    TabContentsWrapper* fullscreened_tab_;

    // True if the current tab is in fullscreen mode.
    bool tab_caused_fullscreen_;

    DISALLOW_COPY_AND_ASSIGN(Browser);
};

#endif //__browser_h__