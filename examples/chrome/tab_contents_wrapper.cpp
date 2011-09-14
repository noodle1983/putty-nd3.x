
#include "tab_contents_wrapper.h"

#include "ui_base/l10n/l10n_util.h"

#include "../wanui_res/resource.h"

#include "infobar_delegate.h"

////////////////////////////////////////////////////////////////////////////////
// TabContentsWrapper, public:

TabContentsWrapper::TabContentsWrapper(TabContents* contents)
: delegate_(NULL),
in_destructor_(false),
tab_contents_(contents)
{
    DCHECK(contents);
    DCHECK(!GetCurrentWrapperForContents(contents));
}

TabContentsWrapper::~TabContentsWrapper()
{
    in_destructor_ = true;

    // Need to tear down infobars before the TabContents goes away.
    //infobar_tab_helper_.reset();
}

string16 TabContentsWrapper::GetDefaultTitle()
{
    return ui::GetStringUTF16(IDS_DEFAULT_TAB_TITLE);
}

TabContentsWrapper* TabContentsWrapper::Clone()
{
    TabContents* new_contents = tab_contents()->Clone();
    TabContentsWrapper* new_wrapper = new TabContentsWrapper(new_contents);

    //new_wrapper->extension_tab_helper()->CopyStateFrom(
    //    *extension_tab_helper_.get());
    return new_wrapper;
}

void TabContentsWrapper::CaptureSnapshot()
{
    //Send(new ChromeViewMsg_CaptureSnapshot(routing_id()));
}

// static
TabContentsWrapper* TabContentsWrapper::GetCurrentWrapperForContents(
    TabContents* contents)
{
    return NULL;
    //TabContentsWrapper** wrapper =
    //    property_accessor()->GetProperty(contents->property_bag());

    //return wrapper ? *wrapper : NULL;
}

// static
const TabContentsWrapper* TabContentsWrapper::GetCurrentWrapperForContents(
    const TabContents* contents)
{
    return NULL;
    //TabContentsWrapper* const* wrapper =
    //    property_accessor()->GetProperty(contents->property_bag());

    //return wrapper ? *wrapper : NULL;
}

////////////////////////////////////////////////////////////////////////////////
// TabContentsWrapper implementation:

void TabContentsWrapper::RenderViewCreated(RenderViewHost* render_view_host)
{
    //UpdateAlternateErrorPageURL(render_view_host);
}

void TabContentsWrapper::RenderViewGone()
{
    // Tell the view that we've crashed so it can prepare the sad tab page.
    // Only do this if we're not in browser shutdown, so that TabContents
    // objects that are not in a browser (e.g., HTML dialogs) and thus are
    // visible do not flash a sad tab page.
    //if(browser_shutdown::GetShutdownType() == browser_shutdown::NOT_VALID)
    //{
    //    tab_contents()->view()->OnTabCrashed(
    //        tab_contents()->crashed_status(), tab_contents()->crashed_error_code());
    //}
}

void TabContentsWrapper::DidBecomeSelected()
{
    //WebCacheManager::GetInstance()->ObserveActivity(
    //    tab_contents()->GetRenderProcessHost()->id());
}

void TabContentsWrapper::TabContentsDestroyed(TabContents* tab)
{
    // Destruction of the TabContents should only be done by us from our
    // destructor. Otherwise it's very likely we (or one of the helpers we own)
    // will attempt to access the TabContents and we'll crash.
    DCHECK(in_destructor_);
}

////////////////////////////////////////////////////////////////////////////////
// Internal helpers

void TabContentsWrapper::OnSnapshot(const SkBitmap& bitmap)
{
    //NotificationService::current()->Notify(
    //    chrome::NOTIFICATION_TAB_SNAPSHOT_TAKEN,
    //    Source<TabContentsWrapper>(this),
    //    Details<const SkBitmap>(&bitmap));
}

void TabContentsWrapper::ExitFullscreenMode()
{
    //Send(new ViewMsg_ExitFullscreen(routing_id()));
}