
#ifndef __view_menu_scroll_view_container_h__
#define __view_menu_scroll_view_container_h__

#pragma once

#include "../../view/view.h"

namespace view
{

    class SubmenuView;

    // MenuScrollViewContainer contains the SubmenuView (through a MenuScrollView)
    // and two scroll buttons. The scroll buttons are only visible and enabled if
    // the preferred height of the SubmenuView is bigger than our bounds.
    class MenuScrollViewContainer : public View
    {
    public:
        explicit MenuScrollViewContainer(SubmenuView* content_view);

        // Returns the buttons for scrolling up/down.
        View* scroll_down_button() const { return scroll_down_button_; }
        View* scroll_up_button() const { return scroll_up_button_; }

        // View overrides.
        virtual void OnPaintBackground(gfx::Canvas* canvas);
        virtual void Layout();
        virtual gfx::Size GetPreferredSize();
        virtual void GetAccessibleState(AccessibleViewState* state);

    protected:
        // View override.
        virtual void OnBoundsChanged(const gfx::Rect& previous_bounds);

    private:
        class MenuScrollView;

        // The scroll buttons.
        View* scroll_up_button_;
        View* scroll_down_button_;

        // The scroll view.
        MenuScrollView* scroll_view_;

        // The content view.
        SubmenuView* content_view_;

        DISALLOW_COPY_AND_ASSIGN(MenuScrollViewContainer);
    };

} //namespace view

#endif //__view_menu_scroll_view_container_h__