
#ifndef __default_tab_handler_h__
#define __default_tab_handler_h__

#pragma once

#include <vector>

#include "base/basic_types.h"
#include "base/memory/scoped_ptr.h"

#include "tab_handler.h"
#include "tab_strip_model_delegate.h"
#include "tab_strip_model_observer.h"

// A TabHandler implementation that interacts with the default TabStripModel.
// The intent is that the TabStripModel API is contained at this level, and
// never propagates beyond to the Browser.
class DefaultTabHandler : public TabHandler,
    public TabStripModelDelegate,
    public TabStripModelObserver
{
public:
    explicit DefaultTabHandler(TabHandlerDelegate* delegate);
    virtual ~DefaultTabHandler();

    // Overridden from TabHandler:
    virtual TabStripModel* GetTabStripModel() const;

    // Overridden from TabStripModelDelegate:
    virtual TabContentsWrapper* AddBlankTab(bool foreground);
    virtual TabContentsWrapper* AddBlankTabAt(int index, bool foreground);
    virtual Browser* CreateNewStripWithContents(
        TabContentsWrapper* detached_contents,
        const gfx::Rect& window_bounds,
        const DockInfo& dock_info,
        bool maximize);
    virtual int GetDragActions() const;
    virtual TabContentsWrapper* CreateTabContentsForURL(
        const Url& url,
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
    virtual bool CanRestoreTab();
    virtual void RestoreTab();
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

private:
    TabHandlerDelegate* delegate_;

    scoped_ptr<TabStripModel> model_;

    DISALLOW_COPY_AND_ASSIGN(DefaultTabHandler);
};

#endif //__default_tab_handler_h__