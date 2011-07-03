
#ifndef __view_button_dropdown_h__
#define __view_button_dropdown_h__

#pragma once

#include "base/task.h"

#include "image_button.h"

namespace ui
{
    class MenuModel;
}

namespace view
{

    ////////////////////////////////////////////////////////////////////////////////
    //
    // ButtonDropDown
    //
    // A button class that when pressed (and held) or pressed (and drag down) will
    // display a menu
    //
    ////////////////////////////////////////////////////////////////////////////////
    class ButtonDropDown : public ImageButton
    {
    public:
        // The button's class name.
        static const char kViewClassName[];

        ButtonDropDown(ButtonListener* listener, ui::MenuModel* model);
        virtual ~ButtonDropDown();

        // Overridden from views::View
        virtual bool OnMousePressed(const MouseEvent& event);
        virtual bool OnMouseDragged(const MouseEvent& event);
        virtual void OnMouseReleased(const MouseEvent& event);
        virtual std::string GetClassName() const;
        // Showing the drop down results in a MouseCaptureLost, we need to ignore it.
        virtual void OnMouseCaptureLost() {}
        virtual void OnMouseExited(const MouseEvent& event);
        // Display the right-click menu, as triggered by the keyboard, for instance.
        // Using the member function ShowDropDownMenu for the actual display.
        virtual void ShowContextMenu(const gfx::Point& p, bool is_mouse_gesture);
        virtual void GetAccessibleState(ui::AccessibleViewState* state);

    protected:
        // Overridden from CustomButton. Returns true if the button should become
        // pressed when a user holds the mouse down over the button. For this
        // implementation, both left and right mouse buttons can trigger a change
        // to the PUSHED state.
        virtual bool ShouldEnterPushedState(const MouseEvent& event);

    private:
        // Internal function to show the dropdown menu
        void ShowDropDownMenu(HWND window);

        // The model that populates the attached menu.
        ui::MenuModel* model_;

        // Y position of mouse when left mouse button is pressed
        int y_position_on_lbuttondown_;

        // A factory for tasks that show the dropdown context menu for the button.
        ScopedRunnableMethodFactory<ButtonDropDown> show_menu_factory_;

        DISALLOW_COPY_AND_ASSIGN(ButtonDropDown);
    };

} //namespace view

#endif //__view_button_dropdown_h__