
#include "default_tab_handler.h"

#include "tab_strip_model.h"
#include "browser.h"

////////////////////////////////////////////////////////////////////////////////
// DefaultTabHandler, public:

DefaultTabHandler::DefaultTabHandler(TabHandlerDelegate* delegate)
: delegate_(delegate),
model_(new TabStripModel(this))
{
    model_->AddObserver(this);
}

DefaultTabHandler::~DefaultTabHandler()
{
    // The tab strip should not have any tabs at this point.
    DCHECK(model_->empty());
    model_->RemoveObserver(this);
}

////////////////////////////////////////////////////////////////////////////////
// DefaultTabHandler, TabHandler implementation:

TabStripModel* DefaultTabHandler::GetTabStripModel() const
{
    return model_.get();
}

////////////////////////////////////////////////////////////////////////////////
// DefaultTabHandler, TabStripModelDelegate implementation:

TabContentsWrapper* DefaultTabHandler::AddBlankTab(bool foreground)
{
    return delegate_->AsBrowser()->AddBlankTab(foreground);
}

TabContentsWrapper* DefaultTabHandler::AddBlankTabAt(int index,
                                                     bool foreground)
{
    return delegate_->AsBrowser()->AddBlankTabAt(index, foreground);
}

Browser* DefaultTabHandler::CreateNewStripWithContents(
    TabContentsWrapper* detached_contents,
    const gfx::Rect& window_bounds,
    const DockInfo& dock_info,
    bool maximize)
{
    return delegate_->AsBrowser()->CreateNewStripWithContents(
        detached_contents,
        window_bounds,
        dock_info,
        maximize);
}

int DefaultTabHandler::GetDragActions() const
{
    return delegate_->AsBrowser()->GetDragActions();
}

TabContentsWrapper* DefaultTabHandler::CreateTabContentsForURL(
    const Url& url,
    const Url& referrer,
    bool defer_load) const
{
    return delegate_->AsBrowser()->CreateTabContentsForURL(url,
        referrer,
        defer_load);
}

bool DefaultTabHandler::CanDuplicateContentsAt(int index)
{
    return delegate_->AsBrowser()->CanDuplicateContentsAt(index);
}

void DefaultTabHandler::DuplicateContentsAt(int index)
{
    delegate_->AsBrowser()->DuplicateContentsAt(index);
}

void DefaultTabHandler::CloseFrameAfterDragSession()
{
    delegate_->AsBrowser()->CloseFrameAfterDragSession();
}

void DefaultTabHandler::CreateHistoricalTab(TabContentsWrapper* contents)
{
    delegate_->AsBrowser()->CreateHistoricalTab(contents);
}

bool DefaultTabHandler::RunUnloadListenerBeforeClosing(
    TabContentsWrapper* contents)
{
    return delegate_->AsBrowser()->RunUnloadListenerBeforeClosing(contents);
}

bool DefaultTabHandler::CanCloseContents(std::vector<int>* indices)
{
    return delegate_->AsBrowser()->CanCloseContents(indices);
}

bool DefaultTabHandler::CanBookmarkAllTabs() const
{
    return delegate_->AsBrowser()->CanBookmarkAllTabs();
}

void DefaultTabHandler::BookmarkAllTabs()
{
    delegate_->AsBrowser()->BookmarkAllTabs();
}

bool DefaultTabHandler::CanCloseTab() const
{
    return delegate_->AsBrowser()->CanCloseTab();
}

bool DefaultTabHandler::CanRestoreTab()
{
    return false;
    //return delegate_->AsBrowser()->CanRestoreTab();
}

void DefaultTabHandler::RestoreTab()
{
    //delegate_->AsBrowser()->RestoreTab();
}

bool DefaultTabHandler::LargeIconsPermitted() const
{
    return delegate_->AsBrowser()->LargeIconsPermitted();
}

////////////////////////////////////////////////////////////////////////////////
// DefaultTabHandler, TabStripModelObserver implementation:

void DefaultTabHandler::TabInsertedAt(TabContentsWrapper* contents,
                                      int index,
                                      bool foreground)
{
    delegate_->AsBrowser()->TabInsertedAt(contents, index, foreground);
}

void DefaultTabHandler::TabClosingAt(TabStripModel* tab_strip_model,
                                     TabContentsWrapper* contents,
                                     int index)
{
    delegate_->AsBrowser()->TabClosingAt(tab_strip_model, contents, index);
}

void DefaultTabHandler::TabDetachedAt(TabContentsWrapper* contents, int index)
{
    delegate_->AsBrowser()->TabDetachedAt(contents, index);
}

void DefaultTabHandler::TabDeactivated(TabContentsWrapper* contents)
{
    delegate_->AsBrowser()->TabDeactivated(contents);
}

void DefaultTabHandler::ActiveTabChanged(TabContentsWrapper* old_contents,
                                         TabContentsWrapper* new_contents,
                                         int index,
                                         bool user_gesture)
{
    delegate_->AsBrowser()->ActiveTabChanged(old_contents,
        new_contents,
        index,
        user_gesture);
}

void DefaultTabHandler::TabMoved(TabContentsWrapper* contents,
                                 int from_index,
                                 int to_index)
{
    delegate_->AsBrowser()->TabMoved(contents, from_index, to_index);
}

void DefaultTabHandler::TabReplacedAt(TabStripModel* tab_strip_model,
                                      TabContentsWrapper* old_contents,
                                      TabContentsWrapper* new_contents,
                                      int index)
{
    delegate_->AsBrowser()->TabReplacedAt(tab_strip_model, old_contents,
        new_contents, index);
}

void DefaultTabHandler::TabStripEmpty()
{
    delegate_->AsBrowser()->TabStripEmpty();
}

////////////////////////////////////////////////////////////////////////////////
// TabHandler, public:

// static
TabHandler* TabHandler::CreateTabHandler(TabHandlerDelegate* delegate)
{
    return new DefaultTabHandler(delegate);
}