
#include "tab_menu_model.h"

#include "tab_strip_model.h"

TabMenuModel::TabMenuModel(ui::SimpleMenuModel::Delegate* delegate)
: ui::SimpleMenuModel(delegate)
{
    Build();
}

TabMenuModel::TabMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                           TabStripModel* tab_strip,
                           int index)
                           : ui::SimpleMenuModel(delegate)
{
    Build(tab_strip, index);
}

void TabMenuModel::Build()
{
    //AddItemWithStringId(TabStripModel::CommandNewTab, IDS_TAB_CXMENU_NEWTAB);
    //AddSeparator();
    //AddItemWithStringId(TabStripModel::CommandReload, IDS_TAB_CXMENU_RELOAD);
    //AddItemWithStringId(TabStripModel::CommandDuplicate,
    //    IDS_TAB_CXMENU_DUPLICATE);
    //AddSeparator();
    //AddItemWithStringId(TabStripModel::CommandCloseTab,
    //    IDS_TAB_CXMENU_CLOSETAB);
    //AddItemWithStringId(TabStripModel::CommandCloseOtherTabs,
    //    IDS_TAB_CXMENU_CLOSEOTHERTABS);
    //AddItemWithStringId(TabStripModel::CommandCloseTabsToRight,
    //    IDS_TAB_CXMENU_CLOSETABSTORIGHT);
    //AddSeparator();
    //AddItemWithStringId(TabStripModel::CommandRestoreTab, IDS_RESTORE_TAB);
    //AddItemWithStringId(TabStripModel::CommandBookmarkAllTabs,
    //    IDS_TAB_CXMENU_BOOKMARK_ALL_TABS);
}

void TabMenuModel::Build(TabStripModel* tab_strip, int index)
{
    //bool affects_multiple_tabs =
    //    (tab_strip->IsTabSelected(index) &&
    //    tab_strip->selection_model().selected_indices().size() > 1);
    //AddItemWithStringId(TabStripModel::CommandNewTab, IDS_TAB_CXMENU_NEWTAB);
    //AddSeparator();
    //AddItemWithStringId(TabStripModel::CommandReload, IDS_TAB_CXMENU_RELOAD);
    //AddItemWithStringId(TabStripModel::CommandDuplicate,
    //    IDS_TAB_CXMENU_DUPLICATE);
    //AddSeparator();
    //if(affects_multiple_tabs)
    //{
    //    AddItemWithStringId(TabStripModel::CommandCloseTab,
    //        IDS_TAB_CXMENU_CLOSETABS);
    //}
    //else
    //{
    //    AddItemWithStringId(TabStripModel::CommandCloseTab,
    //        IDS_TAB_CXMENU_CLOSETAB);
    //}
    //AddItemWithStringId(TabStripModel::CommandCloseOtherTabs,
    //    IDS_TAB_CXMENU_CLOSEOTHERTABS);
    //AddItemWithStringId(TabStripModel::CommandCloseTabsToRight,
    //    IDS_TAB_CXMENU_CLOSETABSTORIGHT);
    //AddSeparator();
    //AddItemWithStringId(TabStripModel::CommandRestoreTab, IDS_RESTORE_TAB);
    //AddItemWithStringId(TabStripModel::CommandBookmarkAllTabs,
    //    IDS_TAB_CXMENU_BOOKMARK_ALL_TABS);
}