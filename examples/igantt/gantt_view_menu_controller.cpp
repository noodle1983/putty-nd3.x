
#include "gantt_view_menu_controller.h"

#include "base/memory/scoped_ptr.h"
#include "view/view.h"
#include "view/widget/widget.h"

// static
GanttViewMenuController* GanttViewMenuController::GetInstance()
{
    return Singleton<GanttViewMenuController>::get();
}

GanttViewMenuController::GanttViewMenuController()
{
}

GanttViewMenuController::~GanttViewMenuController()
{
}

enum GanttViewContextMenuCommand
{
    GANTTVIEWCONTEXTMENUCOMMAND_EDIT,
    GANTTVIEWCONTEXTMENUCOMMAND_CREATE_BAR,
    GANTTVIEWCONTEXTMENUCOMMAND_CREATE_LINK
};

void GanttViewMenuController::ShowContextMenuForView(
    view::View* source,
    const gfx::Point& p,
    bool is_mouse_gesture)
{
    scoped_ptr<view::Menu> menu(view::Menu::Create(this,
        view::Menu::TOPLEFT, source->GetWidget()->GetNativeView()));
    menu->AppendMenuItem(GANTTVIEWCONTEXTMENUCOMMAND_EDIT, L"指针模式",
        view::Menu::RADIO);
    menu->AppendMenuItem(GANTTVIEWCONTEXTMENUCOMMAND_CREATE_BAR,
        L"创建节点模式", view::Menu::RADIO);
    menu->AppendMenuItem(GANTTVIEWCONTEXTMENUCOMMAND_CREATE_LINK,
        L"创建链接模式", view::Menu::RADIO);
    menu->RunMenuAt(p.x(), p.y());
}

bool GanttViewMenuController::IsItemChecked(int id) const
{
    switch(id)
    {
    case GANTTVIEWCONTEXTMENUCOMMAND_EDIT:
        return true;
    }

    return false;
}

bool GanttViewMenuController::IsCommandEnabled(int id) const
{
    return true;
}

void GanttViewMenuController::ExecuteCommand(int id)
{
}