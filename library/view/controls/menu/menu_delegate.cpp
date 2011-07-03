
#include "menu_delegate.h"

#include "base/logging.h"

#include "ui_base/dragdrop/drag_drop_types.h"

#include "menu_config.h"

namespace view
{

    MenuDelegate::~MenuDelegate() {}

    bool MenuDelegate::IsItemChecked(int id) const
    {
        return false;
    }

    std::wstring MenuDelegate::GetLabel(int id) const
    {
        return std::wstring();
    }

    const gfx::Font& MenuDelegate::GetLabelFont(int id) const
    {
        return MenuConfig::instance().font;
    }

    std::wstring MenuDelegate::GetTooltipText(int id,
        const gfx::Point& screen_loc)
    {
        return std::wstring();
    }

    bool MenuDelegate::GetAccelerator(int id, Accelerator* accelerator)
    {
        return false;
    }

    bool MenuDelegate::ShowContextMenu(MenuItemView* source,
        int id,
        const gfx::Point& p,
        bool is_mouse_gesture)
    {
        return false;
    }

    bool MenuDelegate::SupportsCommand(int id) const
    {
        return true;
    }

    bool MenuDelegate::IsCommandEnabled(int id) const
    {
        return true;
    }

    bool MenuDelegate::GetContextualLabel(int id, std::wstring* out) const
    {
        return false;
    }

    bool MenuDelegate::ShouldCloseAllMenusOnExecute(int id)
    {
        return true;
    }

    void MenuDelegate::ExecuteCommand(int id, int mouse_event_flags)
    {
        ExecuteCommand(id);
    }

    bool MenuDelegate::IsTriggerableEvent(const MouseEvent& e)
    {
        return e.IsLeftMouseButton() || e.IsRightMouseButton();
    }

    bool MenuDelegate::CanDrop(MenuItemView* menu, const ui::OSExchangeData& data)
    {
        return false;
    }

    bool MenuDelegate::GetDropFormats(
        MenuItemView* menu,
        int* formats,
        std::set<ui::OSExchangeData::CustomFormat>* custom_formats)
    {
        return false;
    }

    bool MenuDelegate::AreDropTypesRequired(MenuItemView* menu)
    {
        return false;
    }

    int MenuDelegate::GetDropOperation(MenuItemView* item,
        const DropTargetEvent& event,
        DropPosition* position)
    {
        NOTREACHED() << "If you override CanDrop, you need to override this too";
        return ui::DragDropTypes::DRAG_NONE;
    }

    int MenuDelegate::OnPerformDrop(MenuItemView* menu,
        DropPosition position,
        const DropTargetEvent& event)
    {
        NOTREACHED() << "If you override CanDrop, you need to override this too";
        return ui::DragDropTypes::DRAG_NONE;
    }

    bool MenuDelegate::CanDrag(MenuItemView* menu)
    {
        return false;
    }

    void MenuDelegate::WriteDragData(MenuItemView* sender, ui::OSExchangeData* data)
    {
        NOTREACHED() << "If you override CanDrag, you must override this too.";
    }

    int MenuDelegate::GetDragOperations(MenuItemView* sender)
    {
        NOTREACHED() << "If you override CanDrag, you must override this too.";
        return 0;
    }

    MenuItemView* MenuDelegate::GetSiblingMenu(MenuItemView* menu,
        const gfx::Point& screen_point,
        MenuItemView::AnchorPosition* anchor,
        bool* has_mnemonics,
        MenuButton** button)
    {
        return NULL;
    }

    int MenuDelegate::GetMaxWidthForMenu(MenuItemView* menu)
    {
        // NOTE: this needs to be large enough to accommodate the wrench menu with
        // big fonts.
        return 800;
    }

    void MenuDelegate::WillShowMenu(MenuItemView* menu) {}

    void MenuDelegate::WillHideMenu(MenuItemView* menu) {}

} //namespace view