
#ifndef __view_listbox_h__
#define __view_listbox_h__

#pragma once

#include "view/view.h"

namespace view
{

    class ListboxModel;
    class NativeListboxWrapper;

    // A Listbox is a view that displays multiple rows of fixed strings.
    // Exactly one of these strings is shown as selected at all times.
    class Listbox : public View
    {
    public:
        // The listbox's class name.
        static const char kViewClassName[];

        // An interface implemented by an object to let it know that a listbox
        // selection has changed.
        class Listener
        {
        public:
            // This is called if the user changes the current selection of the
            // listbox.
            virtual void SelectionChanged(Listbox* sender) = 0;
        };

        // Creates a new listbox, given the list of strings. |listener| can be NULL.
        // Listbox does not take ownership of |listener|.
        Listbox(ListboxModel* model);
        virtual ~Listbox();

        // Register |listener| for item change events.
        void set_listener(Listener* listener)
        {
            listener_ = listener;
        }

        // Inform the combo box that its model changed.
        void ModelChanged();

        // Returns the 0-based index of the currently selected row, or -1 if nothing
        // is selected. Note that as soon as a row has been selected once, there will
        // always be a selected row.
        int SelectedRow() const;

        // Selects the specified row. Note that this does NOT call the listener's
        // |ListboxSelectionChanged()| method.
        void SelectRow(int row);

        // Called when the combo box's selection is changed by the user.
        void SelectionChanged();

        // Accessor for |model_|.
        ListboxModel* model() const { return model_; }

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void Layout();
        virtual void OnEnabledChanged();
        virtual void OnPaintFocusBorder(gfx::Canvas* canvas);

    protected:
        // Overridden from View:
        virtual void OnFocus();
        virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
        virtual std::string GetClassName() const;

    private:
        // Our model.
        ListboxModel* model_;

        // Listens to selection changes.
        Listener* listener_;

        // The object that actually implements the table.
        NativeListboxWrapper* native_wrapper_;

        DISALLOW_COPY_AND_ASSIGN(Listbox);
    };

} //namespace view

#endif //__view_listbox_h__