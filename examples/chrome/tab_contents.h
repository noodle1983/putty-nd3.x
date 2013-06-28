
#ifndef __tab_contents_h__
#define __tab_contents_h__

#pragma once

#include <deque>
#include <map>
#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/process_util.h"
#include "base/time.h"
#include "base/win/scoped_handle.h"

#include "page_navigator.h"
//#include "tab_contents_observer.h"
#include "tab_contents_view_views.h"
#include "window_open_disposition.h"

#include "putty_view.h"

namespace gfx
{
    class Rect;
}

class TabContentsDelegate;
class TabContentsView;

// Describes what goes in the main content area of a tab. TabContents is
// the only type of TabContents, and these should be merged together.
class TabContents : public PageNavigator
{
public:
    // Flags passed to the TabContentsDelegate.NavigationStateChanged to tell it
    // what has changed. Combine them to update more than one thing.
    enum InvalidateTypes
    {
        INVALIDATE_URL             = 1 << 0,  // The URL has changed.
        INVALIDATE_TAB             = 1 << 1,  // The favicon, app icon, or crashed
                                              // state changed.
        INVALIDATE_LOAD            = 1 << 2,  // The loading state has changed.
        INVALIDATE_PAGE_ACTIONS    = 1 << 3,  // Page action icons have changed.
        INVALIDATE_TITLE           = 1 << 4,  // The title changed.
    };

    // |base_tab_contents| is used if we want to size the new tab contents view
    // based on an existing tab contents view.  This can be NULL if not needed.
    //
    // The session storage namespace parameter allows multiple render views and
    // tab contentses to share the same session storage (part of the WebStorage
    // spec) space. This is useful when restoring tabs, but most callers should
    // pass in NULL which will cause a new SessionStorageNamespace to be created.
    TabContents(int routing_id, const TabContents* base_tab_contents);
    virtual ~TabContents();

    // Intrinsic tab state -------------------------------------------------------

    TabContentsDelegate* delegate() const { return delegate_; }
    void set_delegate(TabContentsDelegate* delegate);

    // The TabContentsView will never change and is guaranteed non-NULL.
    TabContentsView* view() const
    {
        return view_.get();
    }

	view::View* puttyView() const
	{
		return putty_view_.get();
	}

    // Tab navigation state ------------------------------------------------------

    // Returns the current navigation properties, which if a navigation is
    // pending may be provisional (e.g., the navigation could result in a
    // download, in which case the URL would revert to what it was previously).
    virtual const Url& GetURL() const;
    virtual const string16& GetTitle() const;

    // The max PageID of any page that this TabContents has loaded.  PageIDs
    // increase with each new page that is loaded by a tab.  If this is a
    // TabContents, then the max PageID is kept separately on each SiteInstance.
    // Returns -1 if no PageIDs have yet been seen.
    int32 GetMaxPageID();

    // Updates the max PageID to be at least the given PageID.
    void UpdateMaxPageID(int32 page_id);

    // Defines whether this tab's URL should be displayed in the browser's URL
    // bar. Normally this is true so you can see the URL. This is set to false
    // for the new tab page and related pages so that the URL bar is empty and
    // the user is invited to type into it.
    virtual bool ShouldDisplayURL();

    // Return whether this tab contents is loading a resource, or whether its
    // web_ui is.
    bool IsLoading() const;

    // Returns whether this tab contents is waiting for a first-response for the
    // main resource of the page. This controls whether the throbber state is
    // "waiting" or "loading."
    bool waiting_for_response() const { return waiting_for_response_; }

    // Internal state ------------------------------------------------------------

    // This flag indicates whether the tab contents is currently being
    // screenshotted by the DraggedTabController.
    bool capturing_contents() const { return capturing_contents_; }
    void set_capturing_contents(bool cap) { capturing_contents_ = cap; }

    // Indicates whether this tab should be considered crashed. The setter will
    // also notify the delegate when the flag is changed.
    bool is_crashed() const
    {
        return (crashed_status_==base::TERMINATION_STATUS_PROCESS_CRASHED ||
            crashed_status_==base::TERMINATION_STATUS_ABNORMAL_TERMINATION ||
            crashed_status_==base::TERMINATION_STATUS_PROCESS_WAS_KILLED);
    }
    base::TerminationStatus crashed_status() const { return crashed_status_; }
    int crashed_error_code() const { return crashed_error_code_; }
    void SetIsCrashed(base::TerminationStatus status, int error_code);

    // Whether the tab is in the process of being destroyed.
    // Added as a tentative work-around for focus related bug #4633.  This allows
    // us not to store focus when a tab is being closed.
    bool is_being_destroyed() const { return is_being_destroyed_; }

    // Convenience method for notifying the delegate of a navigation state
    // change. See TabContentsDelegate.
    void NotifyNavigationStateChanged(unsigned changed_flags);

    // Invoked when the tab contents becomes selected. If you override, be sure
    // and invoke super's implementation.
    virtual void DidBecomeSelected();
    base::TimeTicks last_selected_time() const
    {
        return last_selected_time_;
    }

    // Invoked when the tab contents becomes hidden.
    // NOTE: If you override this, call the superclass version too!
    virtual void WasHidden();

    // TODO(brettw) document these.
    virtual void ShowContents();
    virtual void HideContents();

    // Returns true if the before unload and unload listeners need to be
    // fired. The value of this changes over time. For example, if true and the
    // before unload listener is executed and allows the user to exit, then this
    // returns false.
    bool NeedToFireBeforeUnload();

    // Commands ------------------------------------------------------------------

    // Implementation of PageNavigator.

    // Deprecated. Please use the one-argument variant instead.
    // TODO(adriansc): Remove this method once refactoring changed all call sites.
    virtual TabContents* OpenURL(const Url& url,
        const Url& referrer,
        WindowOpenDisposition disposition) OVERRIDE;

    virtual TabContents* OpenURL(const OpenURLParams& params) OVERRIDE;

    // Stop any pending navigation.
    virtual void Stop();

    // Creates a new TabContents with the same state as this one. The returned
    // heap-allocated pointer is owned by the caller.
    virtual TabContents* Clone();

    // Window management ---------------------------------------------------------

    // Adds a new tab or window with the given already-created contents.
    void AddNewContents(TabContents* new_contents,
        WindowOpenDisposition disposition,
        const gfx::Rect& initial_pos,
        bool user_gesture);

    // Views and focus -----------------------------------------------------------
    // TODO(brettw): Most of these should be removed and the caller should call
    // the view directly.

    // Returns the actual window that is focused when this TabContents is shown.
    HWND GetContentNativeView() const;

    // Returns the NativeView associated with this TabContents. Outside of
    // automation in the context of the UI, this is required to be implemented.
    HWND GetNativeView() const;

    // Returns the bounds of this TabContents in the screen coordinate system.
    void GetContainerBounds(gfx::Rect *out) const;

    // Makes the tab the focused window.
    void Focus();

    // Focuses the first (last if |reverse| is true) element in the page.
    // Invoked when this tab is getting the focus through tab traversal (|reverse|
    // is true when using Shift-Tab).
    void FocusThroughTabTraversal(bool reverse);

    // These next two functions are declared on RenderViewHostManager::Delegate
    // but also accessed directly by other callers.

    // Returns true if the location bar should be focused by default rather than
    // the page contents. The view calls this function when the tab is focused
    // to see what it should do.
    virtual bool FocusLocationBarByDefault();

    // Focuses the location bar.
    virtual void SetFocusToLocationBar(bool select_all);

    // Misc state & callbacks ----------------------------------------------------

    // Set the time when we started to create the new tab page.  This time is
    // from before we created this TabContents.
    void set_new_tab_start_time(const base::TimeTicks& time)
    {
        new_tab_start_time_ = time;
    }
    base::TimeTicks new_tab_start_time() const { return new_tab_start_time_; }

    // Notification that tab closing has started.  This can be called multiple
    // times, subsequent calls are ignored.
    void OnCloseStarted();

    // Returns true if underlying TabContentsView should accept drag-n-drop.
    bool ShouldAcceptDragAndDrop() const;

    // A render view-originated drag has ended. Informs the render view host and
    // tab contents delegate.
    void SystemDragEnded();

    // Indicates if this tab was explicitly closed by the user (control-w, close
    // tab menu item...). This is false for actions that indirectly close the tab,
    // such as closing the window.  The setter is maintained by TabStripModel, and
    // the getter only useful from within TAB_CLOSED notification
    void set_closed_by_user_gesture(bool value)
    {
        closed_by_user_gesture_ = value;
    }
    bool closed_by_user_gesture() const { return closed_by_user_gesture_; }

    int content_restrictions() const { return content_restrictions_; }
    void SetContentRestrictions(int restrictions);

protected:
    friend class TabContentsObserver;

    // Add and remove observers for page navigation notifications. Adding or
    // removing multiple times has no effect. The order in which notifications
    // are sent to observers is undefined. Clients must be sure to remove the
    // observer before they go away.
    void AddObserver(TabContentsObserver* observer);
    void RemoveObserver(TabContentsObserver* observer);

private:
    // Temporary until the view/contents separation is complete.
    friend class TabContentsView;
    friend class TabContentsViewViews;

    // Misc non-view stuff -------------------------------------------------------

    // Helper functions for sending notifications.
    void NotifySwapped();
    void NotifyConnected();
    void NotifyDisconnected();

    // Data for core operation ---------------------------------------------------

    // Delegate for notifying our owner about stuff. Not owned by us.
    TabContentsDelegate* delegate_;
    // The corresponding view.
    scoped_ptr<TabContentsView> view_;

	scoped_ptr<view::PuttyView> putty_view_;

    // Data for loading state ----------------------------------------------------

    // Indicates whether we're currently loading a resource.
    bool is_loading_;

    // Indicates if the tab is considered crashed.
    base::TerminationStatus crashed_status_;
    int crashed_error_code_;

    // See waiting_for_response() above.
    bool waiting_for_response_;

    // Indicates the largest PageID we've seen.  This field is ignored if we are
    // a TabContents, in which case the max page ID is stored separately with
    // each SiteInstance.
    // TODO(brettw) this seems like it can be removed according to the comment.
    int32 max_page_id_;

    // System time at which the current load was started.
    base::TimeTicks current_load_start_;

    // Data for current page -----------------------------------------------------

    // When a title cannot be taken from any entry, this title will be used.
    string16 page_title_when_no_navigation_entry_;

    // Data for misc internal state ----------------------------------------------

    // See capturing_contents() above.
    bool capturing_contents_;

    // See getter above.
    bool is_being_destroyed_;

    // Indicates whether we should notify about disconnection of this
    // TabContents. This is used to ensure disconnection notifications only
    // happen if a connection notification has happened and that they happen only
    // once.
    bool notify_disconnection_;

    // Handle to an event that's set when the page is showing a message box (or
    // equivalent constrained window).  Plugin processes check this to know if
    // they should pump messages then.
    base::win::ScopedHandle message_box_active_;

    // Set to true when there is an active "before unload" dialog.  When true,
    // we've forced the throbber to start in Navigate, and we need to remember to
    // turn it off in OnJavaScriptMessageBoxClosed if the navigation is canceled.
    bool is_showing_before_unload_dialog_;

    // The time that we started to create the new tab page.
    base::TimeTicks new_tab_start_time_;

    // The time that we started to close the tab.
    base::TimeTicks tab_close_start_time_;

    // The time that this tab was last selected.
    base::TimeTicks last_selected_time_;

    // See description above setter.
    bool closed_by_user_gesture_;

    // A list of observers notified when page state changes. Weak references.
    //ObserverList<TabContentsObserver> observers_;

    // Content restrictions, used to disable print/copy etc based on content's
    // (full-page plugins for now only) permissions.
    int content_restrictions_;

    DISALLOW_COPY_AND_ASSIGN(TabContents);
};

#endif //__tab_contents_h__