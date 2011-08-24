
#ifndef __view_native_combobox_view_h__
#define __view_native_combobox_view_h__

#pragma once

#include "native_combobox_wrapper.h"
#include "view/controls/menu/menu_delegate.h"
#include "view/view.h"

namespace gfx
{
    class Canvas;
    class Font;
}

namespace view
{

    class KeyEvent;
    class FocusableBorder;

    // A views/skia only implementation of NativeComboboxWrapper.
    // No platform specific code is used.
    class NativeComboboxView : public View,
        public NativeComboboxWrapper,
        public MenuDelegate
    {
    public:
        explicit NativeComboboxView(Combobox* parent);
        virtual ~NativeComboboxView();

        // View overrides:
        virtual bool OnMousePressed(const MouseEvent& mouse_event);
        virtual bool OnMouseDragged(const MouseEvent& mouse_event);
        virtual bool OnKeyPressed(const KeyEvent& key_event);
        virtual bool OnKeyReleased(const KeyEvent& key_event);
        virtual void OnPaint(gfx::Canvas* canvas);
        virtual void OnFocus();
        virtual void OnBlur();

        // NativeComboboxWrapper overrides:
        virtual void UpdateFromModel();
        virtual void UpdateSelectedItem();
        virtual void UpdateEnabled();
        virtual int GetSelectedItem() const;
        virtual bool IsDropdownOpen() const;
        virtual gfx::Size GetPreferredSize();
        virtual View* GetView();
        virtual void SetFocus();
        virtual bool HandleKeyPressed(const KeyEvent& event);
        virtual bool HandleKeyReleased(const KeyEvent& event);
        virtual void HandleFocus();
        virtual void HandleBlur();
        virtual HWND GetTestingHandle() const;

        // MenuDelegate overrides:
        virtual bool IsItemChecked(int id) const;
        virtual bool IsCommandEnabled(int id) const;
        virtual void ExecuteCommand(int id);
        virtual bool GetAccelerator(int id, Accelerator* accelerator);

        // class name of internal
        static const char kViewClassName[];

    private:
        // Returns the Combobox's font.
        const gfx::Font& GetFont() const;

        // Draw an arrow
        void DrawArrow(gfx::Canvas* canvas,
            int tip_x, int tip_y, int shift_x, int shift_y) const;

        // Draw the selected value of the drop down list
        void PaintText(gfx::Canvas* canvas);

        // Show the drop down list
        void ShowDropDownMenu();

        // The parent combobox, the owner of this object.
        Combobox* combobox_;

        // The reference to the border class. The object is owned by View::border_.
        FocusableBorder* text_border_;

        // Context menu and its content list for the combobox.
        scoped_ptr<MenuItemView> dropdown_list_menu_;

        // Is the drop down list showing
        bool dropdown_open_;

        // Index in the model of the selected item: -1 => none
        int selected_item_;

        // The maximum dimensions of the content in the dropdown
        int content_width_;
        int content_height_;

        DISALLOW_COPY_AND_ASSIGN(NativeComboboxView);
    };

}  // namespace view

#endif //__view_native_combobox_view_h__