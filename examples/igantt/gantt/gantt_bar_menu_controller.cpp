
#include "gantt_bar_menu_controller.h"

#include "base/memory/scoped_ptr.h"
#include "view/view.h"
#include "view/widget/widget.h"

// static
GanttBarMenuController* GanttBarMenuController::GetInstance()
{
    return Singleton<GanttBarMenuController>::get();
}

GanttBarMenuController::GanttBarMenuController()
{
}

GanttBarMenuController::~GanttBarMenuController()
{
}

enum GanttBarContextMenuCommand
{
    GANTTBARCONTEXTMENUCOMMAND_EDIT,
    GANTTBARCONTEXTMENUCOMMAND_DELETE,
};

void GanttBarMenuController::ShowContextMenuForView(
    view::View* source,
    const gfx::Point& p,
    bool is_mouse_gesture)
{
    scoped_ptr<view::Menu> menu(view::Menu::Create(this,
        view::Menu::TOPLEFT, source->GetWidget()->GetNativeView()));
    menu->AppendDelegateMenuItem(GANTTBARCONTEXTMENUCOMMAND_EDIT);
    menu->AppendDelegateMenuItem(GANTTBARCONTEXTMENUCOMMAND_DELETE);
    menu->RunMenuAt(p.x(), p.y());
}

std::wstring GanttBarMenuController::GetLabel(int id) const
{
    std::wstring label;
    switch(id)
    {
    case GANTTBARCONTEXTMENUCOMMAND_EDIT:
        label = L"编辑数据...";
        break;
    case GANTTBARCONTEXTMENUCOMMAND_DELETE:
        label = L"删除节点";
        break;
    default:
        NOTREACHED() << "Invalid GanttBar Context Menu command!";
    }

    return label;
}

bool GanttBarMenuController::IsCommandEnabled(int id) const
{
    return true;
}

void GanttBarMenuController::ExecuteCommand(int id)
{
}