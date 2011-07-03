
#ifndef __view_menu_model_adapter_h__
#define __view_menu_model_adapter_h__

#pragma once

#include <map>

#include "menu_delegate.h"

namespace ui
{
    class MenuModel;
}

namespace view
{
    class MenuItemView;

    // This class wraps an instance of ui::MenuModel with the
    // view::MenuDelegate interface required by view::MenuItemView.
    class MenuModelAdapter : public MenuDelegate
    {
    public:
        // The caller retains ownership of the ui::MenuModel instance and
        // must ensure it exists for the lifetime of the adapter.  The
        // base_id argument is the command id for the first menu item.
        explicit MenuModelAdapter(ui::MenuModel* menu_model);
        virtual ~MenuModelAdapter();

        // Populate a MenuItemView menu with the ui::MenuModel items
        // (including submenus).
        virtual void BuildMenu(MenuItemView* menu);

    protected:
        // view::MenuDelegate implementation.
        virtual void ExecuteCommand(int id);
        virtual void ExecuteCommand(int id, int mouse_event_flags);
        virtual bool GetAccelerator(int id, Accelerator* accelerator);
        virtual std::wstring GetLabel(int id) const;
        virtual const gfx::Font& GetLabelFont(int id) const;
        virtual bool IsCommandEnabled(int id) const;
        virtual bool IsItemChecked(int id) const;
        virtual void SelectionChanged(MenuItemView* menu);
        virtual void WillShowMenu(MenuItemView* menu);
        virtual void WillHideMenu(MenuItemView* menu);

    private:
        // Implementation of BuildMenu().  index_offset is both input and output;
        // on input it contains the offset from index to command id for the model,
        // and on output it contains the offset for the next model.
        void BuildMenuImpl(MenuItemView* menu, ui::MenuModel* model);

        // Container of ui::MenuModel pointers as encountered by preorder
        // traversal.  The first element is always the top-level model
        // passed to the constructor.
        ui::MenuModel* menu_model_;

        // Map MenuItems to MenuModels.  Used to implement WillShowMenu().
        std::map<MenuItemView*, ui::MenuModel*> menu_map_;

        DISALLOW_COPY_AND_ASSIGN(MenuModelAdapter);
    };

} //namespace view

#endif //__view_menu_model_adapter_h__