
#ifndef __view_menu_runner_h__
#define __view_menu_runner_h__

#pragma once

#include "view/controls/menu/menu_item_view.h"

namespace view
{

    class MenuButton;
    class Widget;

    // MenuRunner handles the lifetime of the root MenuItemView. MenuItemView runs a
    // nested message loop, which means care must be taken when the MenuItemView
    // needs to be deleted. MenuRunner makes sure the menu is deleted after the
    // nested message loop completes.
    //
    // MenuRunner can be deleted at any time and will correctly handle deleting the
    // underlying menu.
    //
    // TODO: this is a work around for 57890. If we fix it this class shouldn't be
    // needed.
    class MenuRunner
    {
    public:
        explicit MenuRunner(MenuItemView* menu);
        ~MenuRunner();

        // Runs the menu.
        void RunMenuAt(Widget* parent,
            MenuButton* button,
            const gfx::Rect& bounds,
            MenuItemView::AnchorPosition anchor,
            bool has_mnemonics);

    private:
        class Holder;

        Holder* holder_;

        DISALLOW_COPY_AND_ASSIGN(MenuRunner);
    };

} //namespace view

#endif //__view_menu_runner_h__