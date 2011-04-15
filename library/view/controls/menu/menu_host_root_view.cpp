
#include "menu_host_root_view.h"

#include "menu_controller.h"
#include "submenu_view.h"

namespace view
{

    MenuHostRootView::MenuHostRootView(Widget* widget,
        SubmenuView* submenu)
        : RootView(widget),
        submenu_(submenu),
        forward_drag_to_menu_controller_(true) {}

    bool MenuHostRootView::OnMousePressed(const MouseEvent& event)
    {
        forward_drag_to_menu_controller_ =
            ((event.x()<0 || event.y()<0 || event.x()>=width() ||
            event.y()>=height()) || !RootView::OnMousePressed(event));
        if(forward_drag_to_menu_controller_ && GetMenuController())
        {
            GetMenuController()->OnMousePressed(submenu_, event);
        }
        return true;
    }

    bool MenuHostRootView::OnMouseDragged(const MouseEvent& event)
    {
        if(forward_drag_to_menu_controller_ && GetMenuController())
        {
            GetMenuController()->OnMouseDragged(submenu_, event);
            return true;
        }
        return RootView::OnMouseDragged(event);
    }

    void MenuHostRootView::OnMouseReleased(const MouseEvent& event)
    {
        RootView::OnMouseReleased(event);
        if(forward_drag_to_menu_controller_ && GetMenuController())
        {
            forward_drag_to_menu_controller_ = false;
            GetMenuController()->OnMouseReleased(submenu_, event);
        }
    }

    void MenuHostRootView::OnMouseMoved(const MouseEvent& event)
    {
        RootView::OnMouseMoved(event);
        if(GetMenuController())
        {
            GetMenuController()->OnMouseMoved(submenu_, event);
        }
    }

    bool MenuHostRootView::OnMouseWheel(const MouseWheelEvent& event)
    {
        // RootView::OnMouseWheel forwards to the focused view. We don't have a
        // focused view, so we need to override this then forward to the menu.
        return submenu_->OnMouseWheel(event);
    }

    MenuController* MenuHostRootView::GetMenuController()
    {
        return submenu_->GetMenuItem()->GetMenuController();
    }

} //namespace view