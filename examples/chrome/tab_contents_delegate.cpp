
#include "tab_contents_delegate.h"

#include "base/logging.h"
#include "base/memory/singleton.h"

#include "ui_gfx/rect.h"

#include "tab_contents.h"

TabContentsDelegate::TabContentsDelegate() {}

TabContents* TabContentsDelegate::OpenURLFromTab(
    TabContents* source,
    const Url& url,
    const Url& referrer,
    WindowOpenDisposition disposition)
{
    return OpenURLFromTab(source, OpenURLParams(url, referrer, disposition));
}

TabContents* TabContentsDelegate::OpenURLFromTab(TabContents* source,
                                                 const OpenURLParams& params)
{
    return NULL;
}

void TabContentsDelegate::NavigationStateChanged(const TabContents* source,
                                                 unsigned changed_flags) {}

void TabContentsDelegate::AddNewContents(TabContents* source,
                                         TabContents* new_contents,
                                         WindowOpenDisposition disposition,
                                         const gfx::Rect& initial_pos,
                                         bool user_gesture) {}

void TabContentsDelegate::ActivateContents(TabContents* contents) {}

void TabContentsDelegate::DeactivateContents(TabContents* contents) {}

void TabContentsDelegate::LoadingStateChanged(TabContents* source) {}

void TabContentsDelegate::LoadProgressChanged(double progress) {}

void TabContentsDelegate::CloseContents(TabContents* source) {}

void TabContentsDelegate::MoveContents(TabContents* source,
                                       const gfx::Rect& pos) {}

void TabContentsDelegate::DetachContents(TabContents* source) {}

bool TabContentsDelegate::IsPopupOrPanel(const TabContents* source) const
{
    return false;
}

bool TabContentsDelegate::ShouldFocusConstrainedWindow()
{
    return true;
}

void TabContentsDelegate::WillShowConstrainedWindow(TabContents* source) {}

void TabContentsDelegate::UpdateTargetURL(TabContents* source,
                                          const Url& url) {}

void TabContentsDelegate::ContentsMouseEvent(
    TabContents* source, const gfx::Point& location, bool motion) {}

void TabContentsDelegate::ContentsZoomChange(bool zoom_in) {}

bool TabContentsDelegate::CanReloadContents(TabContents* source) const
{
    return true;
}

void TabContentsDelegate::WillRunBeforeUnloadConfirm() {}

bool TabContentsDelegate::ShouldSuppressDialogs()
{
    return false;
}

void TabContentsDelegate::BeforeUnloadFired(TabContents* tab,
                                            bool proceed,
                                            bool* proceed_to_fire_unload)
{
    *proceed_to_fire_unload = true;
}

void TabContentsDelegate::SetFocusToLocationBar(bool select_all) {}

bool TabContentsDelegate::ShouldFocusPageAfterCrash()
{
    return true;
}

void TabContentsDelegate::RenderWidgetShowing() {}

bool TabContentsDelegate::TakeFocus(bool reverse)
{
    return false;
}

void TabContentsDelegate::LostCapture() {}

void TabContentsDelegate::SetTabContentBlocked(
    TabContents* contents, bool blocked) {}

void TabContentsDelegate::TabContentsFocused(TabContents* tab_content) {}

int TabContentsDelegate::GetExtraRenderViewHeight() const
{
    return 0;
}

bool TabContentsDelegate::HandleContextMenu(const ContextMenuParams& params)
{
    return false;
}

bool TabContentsDelegate::ExecuteContextMenuCommand(int command)
{
    return false;
}

void TabContentsDelegate::HandleMouseUp() {}

void TabContentsDelegate::HandleMouseActivate() {}

void TabContentsDelegate::DragEnded() {}

void TabContentsDelegate::ShowRepostFormWarningDialog(
    TabContents* tab_contents) {}

bool TabContentsDelegate::OnGoToEntryOffset(int offset)
{
    return true;
}

HWND TabContentsDelegate::GetFrameNativeWindow()
{
    return NULL;
}

void TabContentsDelegate::TabContentsCreated(TabContents* new_contents) {}

void TabContentsDelegate::ContentRestrictionsChanged(TabContents* source) {}

void TabContentsDelegate::RendererUnresponsive(TabContents* source) {}

void TabContentsDelegate::RendererResponsive(TabContents* source) {}

void TabContentsDelegate::WorkerCrashed(TabContents* source) {}

void TabContentsDelegate::DidNavigateMainFramePostCommit(
    TabContents* tab) {}

void TabContentsDelegate::DidNavigateToPendingEntry(TabContents* tab) {}

void TabContentsDelegate::ToggleFullscreenModeForTab(
    TabContents* tab, bool enter_fullscreen) {}

TabContentsDelegate::~TabContentsDelegate()
{
    while(!attached_contents_.empty())
    {
        TabContents* tab_contents = *attached_contents_.begin();
        tab_contents->set_delegate(NULL);
    }
    DCHECK(attached_contents_.empty());
}

void TabContentsDelegate::Attach(TabContents* tab_contents)
{
    DCHECK(attached_contents_.find(tab_contents) == attached_contents_.end());
    attached_contents_.insert(tab_contents);
}

void TabContentsDelegate::Detach(TabContents* tab_contents)
{
    DCHECK(attached_contents_.find(tab_contents) != attached_contents_.end());
    attached_contents_.erase(tab_contents);
}