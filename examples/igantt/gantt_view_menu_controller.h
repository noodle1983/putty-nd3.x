
#ifndef __gantt_view_menu_controller_h__
#define __gantt_view_menu_controller_h__

#pragma once

#include "base/memory/singleton.h"
#include "view/context_menu_controller.h"
#include "view/controls/menu/menu.h"

class GanttViewMenuController : public view::ContextMenuController,
    public view::Menu::Delegate
{
public:
    static GanttViewMenuController* GetInstance();

    // Overriden from view::ContextMenuController
    virtual void ShowContextMenuForView(view::View* source,
        const gfx::Point& p, bool is_mouse_gesture);

    // Overriden from view::Menu::Delegate
    virtual bool IsItemChecked(int id) const;
    virtual bool IsCommandEnabled(int id) const;
    virtual void ExecuteCommand(int id);

private:
    GanttViewMenuController();
    virtual ~GanttViewMenuController();

    friend struct DefaultSingletonTraits<GanttViewMenuController>;
    DISALLOW_COPY_AND_ASSIGN(GanttViewMenuController);
};

#endif //__gantt_view_menu_controller_h__