
#ifndef __view_combobox_h__
#define __view_combobox_h__

#pragma once

#include "view/view.h"

namespace ui
{
    class ComboboxModel;
}

namespace view
{

    class NativeComboboxWrapper;

    // A non-editable combo-box.
    class Combobox : public View
    {
    public:
        // The combobox's class name.
        static const char kViewClassName[];

        class Listener
        {
        public:
            // This is invoked once the selected item changed.
            virtual void ItemChanged(Combobox* combo_box,
                int prev_index, int new_index) = 0;

        protected:
            virtual ~Listener() {}
        };

        // |model| is not owned by the combo box.
        explicit Combobox(ui::ComboboxModel* model);
        virtual ~Combobox();

        // Register |listener| for item change events.
        void set_listener(Listener* listener)
        {
            listener_ = listener;
        }

        // Inform the combo box that its model changed.
        void ModelChanged();

        // Gets/Sets the selected item.
        int selected_item() const { return selected_item_; }
        void SetSelectedItem(int index);

        // Called when the combo box's selection is changed by the user.
        void SelectionChanged();

        // Accessor for |model_|.
        ui::ComboboxModel* model() const { return model_; }

        // Set the accessible name of the combo box.
        void SetAccessibleName(const string16& name);

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void Layout();
        virtual void OnEnabledChanged();
        virtual bool SkipDefaultKeyEventProcessing(const KeyEvent& e);
        virtual void OnPaintFocusBorder(gfx::Canvas* canvas);
        virtual bool OnKeyPressed(const KeyEvent& e);
        virtual bool OnKeyReleased(const KeyEvent& e);
        virtual void OnFocus();
        virtual void OnBlur();
        virtual void GetAccessibleState(ui::AccessibleViewState* state);

    protected:
        // Overridden from View:
        virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
        virtual std::string GetClassName() const;

        // The object that actually implements the native combobox.
        NativeComboboxWrapper* native_wrapper_;

    private:
        // Our model.
        ui::ComboboxModel* model_;

        // Item change listener.
        Listener* listener_;

        // The current selection.
        int selected_item_;

        // The accessible name of the text field.
        string16 accessible_name_;

        DISALLOW_COPY_AND_ASSIGN(Combobox);
    };

} //namespace view

#endif //__view_combobox_h__