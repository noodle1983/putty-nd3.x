
#ifndef __gantt_bar_menu_controller_h__
#define __gantt_bar_menu_controller_h__

#pragma once

#include "base/memory/singleton.h"
#include "view/context_menu_controller.h"
#include "view/controls/menu/menu.h"

class GanttBarMenuController : public view::ContextMenuController,
    public view::Menu::Delegate
{
public:
    static GanttBarMenuController* GetInstance();

    // Overriden from view::ContextMenuController
    virtual void ShowContextMenuForView(view::View* source,
        const gfx::Point& p, bool is_mouse_gesture);

    // Overriden from view::Menu::Delegate
    virtual std::wstring GetLabel(int id) const;
    virtual bool IsCommandEnabled(int id) const;
    virtual void ExecuteCommand(int id);

private:
    GanttBarMenuController();
    virtual ~GanttBarMenuController();

    friend struct DefaultSingletonTraits<GanttBarMenuController>;
    DISALLOW_COPY_AND_ASSIGN(GanttBarMenuController);
};

#endif //__gantt_bar_menu_controller_h__