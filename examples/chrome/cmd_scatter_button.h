#ifndef CMD_SCATTER_BUTTON_H
#define CMD_SCATTER_BUTTON_H

#include "base/utf_string_conversions.h"

#include "ui_gfx/image/image.h"

#include "ui_base/models/simple_menu_model.h"
#include "ui_base/resource/app_res_ids.h"
#include "ui_base/resource/resource_bundle.h"

#include "view/controls/button/button_dropdown.h"
#include "view/controls/button/checkbox.h"
#include "view/controls/button/image_button.h"
#include "view/controls/button/menu_button.h"
#include "view/controls/button/radio_button.h"
#include "view/controls/button/text_button.h"
#include "view/controls/menu/menu_2.h"
#include "view/controls/menu/view_menu_delegate.h"
#include "view/layout/box_layout.h"

class CmdScatterMenuModel : public ui::SimpleMenuModel,
    public ui::SimpleMenuModel::Delegate
{
public:
    CmdScatterMenuModel();

    void RunMenuAt(const gfx::Point& point);

    // Overridden from ui::SimpleMenuModel::Delegate:
    virtual bool IsCommandIdChecked(int command_id) const;
    virtual bool IsCommandIdEnabled(int command_id) const;
    virtual bool GetAcceleratorForCommandId(
        int command_id,
        ui::Accelerator* accelerator);
    virtual void ExecuteCommand(int command_id);

private:
    enum
    {
        kGroupMakeDecision,
    };

    enum
    {
        kCommandDoSomething,
        kCommandSelectAscii,
        kCommandSelectUtf8,
        kCommandSelectUtf16,
        kCommandCheckApple,
        kCommandCheckOrange,
        kCommandCheckKiwi,
        kCommandGoHome,
    };

    scoped_ptr<view::Menu2> menu_;
    scoped_ptr<ui::SimpleMenuModel> submenu_;
    std::set<int> checked_fruits_;
    int current_encoding_command_id_;

    DISALLOW_COPY_AND_ASSIGN(CmdScatterMenuModel);
};

class CmdScatterMenuButton : public view::MenuButton,
    public view::ViewMenuDelegate
{
public:
    CmdScatterMenuButton(const std::wstring& test, bool show_menu_marker);
    virtual ~CmdScatterMenuButton();

private:
    // Overridden from views::ViewMenuDelegate:
    virtual void RunMenu(view::View* source, const gfx::Point& point);

    scoped_ptr<CmdScatterMenuModel> menu_model_;
    DISALLOW_COPY_AND_ASSIGN(CmdScatterMenuButton);
};


#endif /*CMD_SCATTER_BUTTON_H */

