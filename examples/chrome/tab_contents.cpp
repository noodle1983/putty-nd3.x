
#include "tab_contents.h"

#include <cmath>

#include "base/metric/histogram.h"

#include "ui_gfx/size.h"

#include "tab_contents_delegate.h"
#include "tab_contents_view.h"

#include "putty_view.h"

// Cross-Site Navigations
//
// If a TabContents is told to navigate to a different web site (as determined
// by SiteInstance), it will replace its current RenderViewHost with a new
// RenderViewHost dedicated to the new SiteInstance.  This works as follows:
//
// - Navigate determines whether the destination is cross-site, and if so,
//   it creates a pending_render_view_host_ and moves into the PENDING
//   RendererState.
// - The pending RVH is "suspended," so that no navigation messages are sent to
//   its renderer until the onbeforeunload JavaScript handler has a chance to
//   run in the current RVH.
// - The pending RVH tells CrossSiteRequestManager (a thread-safe singleton)
//   that it has a pending cross-site request.  ResourceDispatcherHost will
//   check for this when the response arrives.
// - The current RVH runs its onbeforeunload handler.  If it returns false, we
//   cancel all the pending logic and go back to NORMAL.  Otherwise we allow
//   the pending RVH to send the navigation request to its renderer.
// - ResourceDispatcherHost receives a ResourceRequest on the IO thread.  It
//   checks CrossSiteRequestManager to see that the RVH responsible has a
//   pending cross-site request, and then installs a CrossSiteEventHandler.
// - When RDH receives a response, the BufferedEventHandler determines whether
//   it is a download.  If so, it sends a message to the new renderer causing
//   it to cancel the request, and the download proceeds in the download
//   thread.  For now, we stay in a PENDING state (with a pending RVH) until
//   the next DidNavigate event for this TabContents.  This isn't ideal, but it
//   doesn't affect any functionality.
// - After RDH receives a response and determines that it is safe and not a
//   download, it pauses the response to first run the old page's onunload
//   handler.  It does this by asynchronously calling the OnCrossSiteResponse
//   method of TabContents on the UI thread, which sends a SwapOut message
//   to the current RVH.
// - Once the onunload handler is finished, a SwapOut_ACK message is sent to
//   the ResourceDispatcherHost, who unpauses the response.  Data is then sent
//   to the pending RVH.
// - The pending renderer sends a FrameNavigate message that invokes the
//   DidNavigate method.  This replaces the current RVH with the
//   pending RVH and goes back to the NORMAL RendererState.
// - The previous renderer is kept swapped out in RenderViewHostManager in case
//   the user goes back.  The process only stays live if another tab is using
//   it, but if so, the existing frame relationships will be maintained.

namespace
{

    // Amount of time we wait between when a key event is received and the renderer
    // is queried for its state and pushed to the NavigationEntry.
    const int kQueryStateDelay = 5000;

    const int kSyncWaitDelay = 40;

    static const char kDotGoogleDotCom[] = ".google.com";

    BOOL CALLBACK InvalidateWindow(HWND hwnd, LPARAM lparam)
    {
        // Note: erase is required to properly paint some widgets borders. This can
        // be seen with textfields.
        InvalidateRect(hwnd, NULL, TRUE);
        return TRUE;
    }

}


// TabContents ----------------------------------------------------------------

TabContents::TabContents(int routing_id, const TabContents* base_tab_contents)
: delegate_(NULL),
//view_(new TabContentsViewViews(this)),
putty_view_(new view::PuttyView()),
is_loading_(false),
crashed_status_(base::TERMINATION_STATUS_STILL_RUNNING),
crashed_error_code_(0),
waiting_for_response_(false),
max_page_id_(-1),
capturing_contents_(false),
is_being_destroyed_(false),
notify_disconnection_(false),
message_box_active_(CreateEvent(NULL, TRUE, FALSE, NULL)),
is_showing_before_unload_dialog_(false),
closed_by_user_gesture_(false),
content_restrictions_(0)
{
    //render_manager_.Init(browser_context, site_instance, routing_id);

    // We have the initial size of the view be based on the size of the passed in
    // tab contents (normally a tab from the same window).
    //view_->CreateView(base_tab_contents ?
    //    base_tab_contents->view()->GetContainerSize() : gfx::Size(800,480));
}

TabContents::~TabContents()
{
    is_being_destroyed_ = true;

    NotifyDisconnected();

    // OnCloseStarted isn't called in unit tests.
    if(!tab_close_start_time_.is_null())
    {
        UMA_HISTOGRAM_TIMES("Tab.Close",
            base::TimeTicks::Now() - tab_close_start_time_);
    }

    //FOR_EACH_OBSERVER(TabContentsObserver, observers_, TabContentsDestroyed());

    set_delegate(NULL);
}

// TODO(cbentzel): Either remove the debugging code, or rename to SetDelegate.
void TabContents::set_delegate(TabContentsDelegate* delegate)
{
    if(delegate == delegate_)
    {
        return;
    }
    if(delegate_)
    {
        delegate_->Detach(this);
    }
    delegate_ = delegate;
    if(delegate_)
    {
        delegate_->Attach(this);
    }
}


view::View* TabContents::puttyView() const
{
	return putty_view_.get();
}

const Url& TabContents::GetURL() const
{
    // We may not have a navigation entry yet
    //NavigationEntry* entry = controller_.GetActiveEntry();
    return /*entry ? entry->virtual_url() : */Url::EmptyGURL();
}

const string16& TabContents::GetTitle() const
{
    // Transient entries take precedence. They are used for interstitial pages
    // that are shown on top of existing pages.
    //NavigationEntry* entry = controller_.GetTransientEntry();
    //std::string accept_languages =
    //    content::GetContentClient()->browser()->GetAcceptLangs(this);
    //if(entry)
    //{
    //    return entry->GetTitleForDisplay(accept_languages);
    //}
    //WebUI* our_web_ui = render_manager_.pending_web_ui() ?
    //    render_manager_.pending_web_ui() : render_manager_.web_ui();
    //if(our_web_ui)
    //{
    //    // Don't override the title in view source mode.
    //    entry = controller_.GetActiveEntry();
    //    if(!(entry && entry->IsViewSourceMode()))
    //    {
    //        // Give the Web UI the chance to override our title.
    //        const string16& title = our_web_ui->overridden_title();
    //        if(!title.empty())
    //        {
    //            return title;
    //        }
    //    }
    //}

    //// We use the title for the last committed entry rather than a pending
    //// navigation entry. For example, when the user types in a URL, we want to
    //// keep the old page's title until the new load has committed and we get a new
    //// title.
    //entry = controller_.GetLastCommittedEntry();
    //if(entry)
    //{
    //    return entry->GetTitleForDisplay(accept_languages);
    //}

    // |page_title_when_no_navigation_entry_| is finally used
    // if no title cannot be retrieved.
    return putty_view_->getWinTitle();
}

int32 TabContents::GetMaxPageID()
{
    //if(GetSiteInstance())
    //{
    //    return GetSiteInstance()->max_page_id();
    //}
    //else
    {
        return max_page_id_;
    }
}

void TabContents::UpdateMaxPageID(int32 page_id)
{
    // Ensure both the SiteInstance and RenderProcessHost update their max page
    // IDs in sync. Only TabContents will also have site instances, except during
    // testing.
    //if(GetSiteInstance())
    //{
    //    GetSiteInstance()->UpdateMaxPageID(page_id);
    //}
    //GetRenderProcessHost()->UpdateMaxPageID(page_id);
}

bool TabContents::ShouldDisplayURL()
{
    //// Don't hide the url in view source mode and with interstitials.
    //NavigationEntry* entry = controller_.GetActiveEntry();
    //if(entry && (entry->IsViewSourceMode() ||
    //    entry->page_type()==INTERSTITIAL_PAGE))
    //{
    //    return true;
    //}

    //// We always display the URL for non-WebUI URLs to prevent spoofing.
    //if(entry && !content::WebUIFactory::Get()->HasWebUIScheme(entry->url()))
    //{
    //    return true;
    //}

    //WebUI* web_ui = GetWebUIForCurrentState();
    //if(web_ui)
    //{
    //    return !web_ui->should_hide_url();
    //}
    return true;
}

bool TabContents::IsLoading() const
{
    return putty_view_->isLoading(); /*|| (web_ui() && web_ui()->IsLoading())*/
}

void TabContents::AddObserver(TabContentsObserver* observer)
{
    //observers_.AddObserver(observer);
}

void TabContents::RemoveObserver(TabContentsObserver* observer)
{
    //observers_.RemoveObserver(observer);
}

void TabContents::SetIsCrashed(base::TerminationStatus status, int error_code)
{
    if(status == crashed_status_)
    {
        return;
    }

    crashed_status_ = status;
    crashed_error_code_ = error_code;
    NotifyNavigationStateChanged(INVALIDATE_TAB);
}

void TabContents::NotifyNavigationStateChanged(unsigned changed_flags)
{
    if(delegate_)
    {
        delegate_->NavigationStateChanged(this, changed_flags);
    }
}

void TabContents::DidBecomeSelected()
{
    last_selected_time_ = base::TimeTicks::Now();

    //RenderWidgetHostView* rwhv = GetRenderWidgetHostView();
    //if(rwhv)
    //{
    //    rwhv->DidBecomeSelected();
    //}

    last_selected_time_ = base::TimeTicks::Now();

    //FOR_EACH_OBSERVER(TabContentsObserver, observers_, DidBecomeSelected());
}

void TabContents::WasHidden()
{
    if(!capturing_contents())
    {
        // |render_view_host()| can be NULL if the user middle clicks a link to open
        // a tab in then background, then closes the tab before selecting it.  This
        // is because closing the tab calls TabContents::Destroy(), which removes
        // the |render_view_host()|; then when we actually destroy the window,
        // OnWindowPosChanged() notices and calls HideContents() (which calls us).
        //RenderWidgetHostView* rwhv = GetRenderWidgetHostView();
        //if(rwhv)
        //{
        //    rwhv->WasHidden();
        //}
		putty_view_->SetVisible(false);
    }
}

void TabContents::ShowContents()
{
    //RenderWidgetHostView* rwhv = GetRenderWidgetHostView();
    //if(rwhv)
    //{
    //    rwhv->DidBecomeSelected();
    //}
	putty_view_->SetVisible(true);
}

void TabContents::HideContents()
{
    // TODO(pkasting): http://b/1239839  Right now we purposefully don't call
    // our superclass HideContents(), because some callers want to be very picky
    // about the order in which these get called.  In addition to making the code
    // here practically impossible to understand, this also means we end up
    // calling TabContents::WasHidden() twice if callers call both versions of
    // HideContents() on a TabContents.
    WasHidden();
}

bool TabContents::NeedToFireBeforeUnload()
{
    // TODO(creis): Should we fire even for interstitial pages?
    //return notify_disconnection() &&
    //    !showing_interstitial_page() &&
    //    !render_view_host()->SuddenTerminationAllowed();
    return false;
}

// TODO(adriansc): Remove this method once refactoring changed all call sites.
TabContents* TabContents::OpenURL(const Url& url,
                                  const Url& referrer,
                                  WindowOpenDisposition disposition)
{
    return OpenURL(OpenURLParams(url, referrer, disposition));
}

TabContents* TabContents::OpenURL(const OpenURLParams& params)
{
    if(delegate_)
    {
        TabContents* new_contents = delegate_->OpenURLFromTab(this, params);
        // Notify observers.
        //FOR_EACH_OBSERVER(TabContentsObserver, observers_,
        //    DidOpenURL(params.url, params.referrer,
        //    params.disposition));
        return new_contents;
    }
    return NULL;
}

void TabContents::Stop()
{
    //render_manager_.Stop();
    //FOR_EACH_OBSERVER(TabContentsObserver, observers_, StopNavigation());
}

TabContents* TabContents::Clone()
{
    // We create a new SiteInstance so that the new tab won't share processes
    // with the old one. This can be changed in the future if we need it to share
    // processes for some reason.
    TabContents* tc = new TabContents(/*MSG_ROUTING_NONE*/-2, this);
    //tc->controller().CopyStateFrom(controller_);
    return tc;
}

void TabContents::AddNewContents(TabContents* new_contents,
                                 WindowOpenDisposition disposition,
                                 const gfx::Rect& initial_pos,
                                 bool user_gesture)
{
    if(!delegate_)
    {
        return;
    }

    delegate_->AddNewContents(this, new_contents, disposition,
        initial_pos, user_gesture);
}

HWND TabContents::GetContentNativeView() const
{
    //return view_->GetContentNativeView();
	return putty_view_->getNativeView();
}

HWND TabContents::GetNativeView() const
{
    //return view_->GetNativeView();
	return putty_view_->getNativeView();
}

void TabContents::GetContainerBounds(gfx::Rect *out) const
{
    //view_->GetContainerBounds(out);
	*out = putty_view_->bounds();
}

void TabContents::Focus()
{
    //view_->Focus();
	return putty_view_->RequestFocus();
}

void TabContents::FocusThroughTabTraversal(bool reverse)
{
    //if(showing_interstitial_page())
    //{
    //    render_manager_.interstitial_page()->FocusThroughTabTraversal(reverse);
    //    return;
    //}
    //render_view_host()->SetInitialFocus(reverse);
}

bool TabContents::FocusLocationBarByDefault()
{
    //WebUI* web_ui = GetWebUIForCurrentState();
    //if(web_ui)
    //{
    //    return web_ui->focus_location_bar_by_default();
    //}
    //NavigationEntry* entry = controller_.GetActiveEntry();
    //if(entry && entry->url() == GURL(chrome::kAboutBlankURL))
    //{
    //    return true;
    //}
    return false;
}

void TabContents::SetFocusToLocationBar(bool select_all)
{
    //if(delegate())
    //{
    //    delegate()->SetFocusToLocationBar(select_all);
    //}
}

void TabContents::OnCloseStarted()
{
    if(tab_close_start_time_.is_null())
    {
        tab_close_start_time_ = base::TimeTicks::Now();
    }
}

bool TabContents::ShouldAcceptDragAndDrop() const
{
    return true;
}

void TabContents::SystemDragEnded()
{
    //if(render_view_host())
    //{
    //    render_view_host()->DragSourceSystemDragEnded();
    //}
    //if(delegate())
    //{
    //    delegate()->DragEnded();
    //}
}


void TabContents::SetContentRestrictions(int restrictions)
{
    content_restrictions_ = restrictions;
    delegate()->ContentRestrictionsChanged(this);
}

void TabContents::NotifySwapped()
{
    // After sending out a swap notification, we need to send a disconnect
    // notification so that clients that pick up a pointer to |this| can NULL the
    // pointer.  See Bug 1230284.
    notify_disconnection_ = true;
}

void TabContents::NotifyConnected()
{
    notify_disconnection_ = true;
}

void TabContents::NotifyDisconnected()
{
    if(!notify_disconnection_)
    {
        return;
    }

    notify_disconnection_ = false;
}


void TabContents::dupCfg2Global() const
{
	putty_view_->dupCfg2Global();
}

void TabContents::do_copy() const
{
	putty_view_->do_copy();
}

void TabContents::do_paste() const
{
	putty_view_->do_paste();
}

void TabContents::do_restart() const
{
	putty_view_->do_restart();
}

bool TabContents::isDisconnected() const
{
	return putty_view_->isDisconnected();
}

void TabContents::do_reconfig() const
{
	return putty_view_->do_reconfig();
}

void TabContents::do_copyAll() const
{
	return putty_view_->do_copyAll();
}

void TabContents::do_clearScrollbar() const
{
	return putty_view_->do_clearScrollbar();
}

void TabContents::do_log(bool isPressed) const
{
	return putty_view_->do_log(isPressed);
}

void TabContents::do_shortcutEnabler(bool isPressed) const
{
	return putty_view_->do_shortcutEnabler(isPressed);
}

bool TabContents::isLogStarted() const
{
	return putty_view_->isLogStarted();
}

bool TabContents::isShortcutEnabled() const
{
	return putty_view_->isShortcutEnabled();
}