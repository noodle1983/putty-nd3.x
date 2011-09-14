
#ifndef __tab_menu_model_h__
#define __tab_menu_model_h__

#pragma once

#include "ui_base/models/simple_menu_model.h"

class TabStripModel;

// A menu model that builds the contents of the tab context menu. To make sure
// the menu reflects the real state of the tab a new TabMenuModel should be
// created each time the menu is shown.
class TabMenuModel : public ui::SimpleMenuModel
{
public:
    // TODO: nuke this constructor when callers are updated.
    TabMenuModel(ui::SimpleMenuModel::Delegate* delegate);
    TabMenuModel(ui::SimpleMenuModel::Delegate* delegate,
        TabStripModel* tab_strip,
        int index);
    virtual ~TabMenuModel() {}

private:
    // TODO: nuke this when first constructor is removed.
    void Build();
    void Build(TabStripModel* tab_strip, int index);

    DISALLOW_COPY_AND_ASSIGN(TabMenuModel);
};

#endif //__tab_menu_model_h__