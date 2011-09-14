
#ifndef __browser_tab_strip_controller_h__
#define __browser_tab_strip_controller_h__

#pragma once

#include "base/memory/scoped_ptr.h"

//#include "hover_tab_selector.h"
#include "tab_strip_model.h"
#include "tab_strip_controller.h"

class BaseTab;
class BaseTabStrip;
class Browser;
class TabStripSelectionModel;

struct TabRendererData;

// An implementation of TabStripController that sources data from the
// TabContentsWrappers in a TabStripModel.
class BrowserTabStripController : public TabStripController,
    public TabStripModelObserver
{
public:
    BrowserTabStripController(Browser* browser, TabStripModel* model);
    virtual ~BrowserTabStripController();

    void InitFromModel(BaseTabStrip* tabstrip);

    TabStripModel* model() const { return model_; }

    bool IsCommandEnabledForTab(TabStripModel::ContextMenuCommand command_id,
        BaseTab* tab) const;
    bool IsCommandCheckedForTab(TabStripModel::ContextMenuCommand command_id,
        BaseTab* tab) const;
    void ExecuteCommandForTab(TabStripModel::ContextMenuCommand command_id,
        BaseTab* tab);

    // TabStripController implementation:
    virtual int GetCount() const OVERRIDE;
    virtual bool IsValidIndex(int model_index) const OVERRIDE;
    virtual bool IsActiveTab(int model_index) const;
    virtual bool IsTabSelected(int model_index) const OVERRIDE;
    virtual bool IsTabCloseable(int model_index) const OVERRIDE;
    virtual bool IsNewTabPage(int model_index) const OVERRIDE;
    virtual void SelectTab(int model_index) OVERRIDE;
    virtual void ExtendSelectionTo(int model_index) OVERRIDE;
    virtual void ToggleSelected(int model_index) OVERRIDE;
    virtual void AddSelectionFromAnchorTo(int model_index) OVERRIDE;
    virtual void CloseTab(int model_index) OVERRIDE;
    virtual void ShowContextMenuForTab(BaseTab* tab,
        const gfx::Point& p) OVERRIDE;
    virtual void UpdateLoadingAnimations() OVERRIDE;
    virtual int HasAvailableDragActions() const OVERRIDE;
    virtual void OnDropIndexUpdate(int index, bool drop_before) OVERRIDE;
    virtual void PerformDrop(bool drop_before,
                             int index,
                             const Url& url) OVERRIDE;
    virtual bool IsCompatibleWith(BaseTabStrip* other) const OVERRIDE;
    virtual void CreateNewTab() OVERRIDE;
    virtual void ClickActiveTab(int index) OVERRIDE;

    // TabStripModelObserver implementation:
    virtual void TabInsertedAt(TabContentsWrapper* contents,
        int model_index,
        bool active) OVERRIDE;
    virtual void TabDetachedAt(TabContentsWrapper* contents,
        int model_index) OVERRIDE;
    virtual void TabSelectionChanged(
        const TabStripSelectionModel& old_model) OVERRIDE;
    virtual void TabMoved(TabContentsWrapper* contents,
        int from_model_index,
        int to_model_index) OVERRIDE;
    virtual void TabChangedAt(TabContentsWrapper* contents,
        int model_index,
        TabChangeType change_type) OVERRIDE;
    virtual void TabReplacedAt(TabStripModel* tab_strip_model,
        TabContentsWrapper* old_contents,
        TabContentsWrapper* new_contents,
        int model_index) OVERRIDE;
    virtual void TabMiniStateChanged(TabContentsWrapper* contents,
        int model_index) OVERRIDE;
    virtual void TabBlockedStateChanged(TabContentsWrapper* contents,
        int model_index) OVERRIDE;

protected:
    // The context in which SetTabRendererDataFromModel is being called.
    enum TabStatus
    {
        NEW_TAB,
        EXISTING_TAB
    };

    // Sets the TabRendererData from the TabStripModel.
    virtual void SetTabRendererDataFromModel(TabContents* contents,
        int model_index,
        TabRendererData* data,
        TabStatus tab_status);

    const BaseTabStrip* tabstrip() const { return tabstrip_; }

    const Browser* browser() const { return browser_; }

private:
    class TabContextMenuContents;

    // Invokes tabstrip_->SetTabData.
    void SetTabDataAt(TabContentsWrapper* contents, int model_index);

    void StartHighlightTabsForCommand(
        TabStripModel::ContextMenuCommand command_id,
        BaseTab* tab);
    void StopHighlightTabsForCommand(
        TabStripModel::ContextMenuCommand command_id,
        BaseTab* tab);

    TabStripModel* model_;

    BaseTabStrip* tabstrip_;

    // Non-owning pointer to the browser which is using this controller.
    Browser* browser_;

    // If non-NULL it means we're showing a menu for the tab.
    scoped_ptr<TabContextMenuContents> context_menu_contents_;

    // Helper for performing tab selection as a result of dragging over a tab.
    //HoverTabSelector hover_tab_selector_;

    DISALLOW_COPY_AND_ASSIGN(BrowserTabStripController);
};

#endif //__browser_tab_strip_controller_h__