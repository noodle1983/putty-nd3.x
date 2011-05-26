
#ifndef __view_listbox_h__
#define __view_listbox_h__

#pragma once

#include "../../view/view.h"

namespace view
{

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
        Listbox(const std::vector<string16>& strings, Listbox::Listener* listener);
        virtual ~Listbox();

        // Returns the number of rows in the table.
        int GetRowCount() const;

        // Returns the 0-based index of the currently selected row, or -1 if nothing
        // is selected. Note that as soon as a row has been selected once, there will
        // always be a selected row.
        int SelectedRow() const;

        // Selects the specified row. Note that this does NOT call the listener's
        // |ListboxSelectionChanged()| method.
        void SelectRow(int row);

        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual void Layout();
        virtual void SetEnabled(bool enabled);
        virtual void OnPaintFocusBorder(gfx::Canvas* canvas);

    protected:
        // Overridden from View:
        virtual void OnFocus();
        virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
        virtual std::string GetClassName() const;

        virtual NativeListboxWrapper* CreateWrapper();

    private:
        // Data stored in the listbox.
        std::vector<string16> strings_;

        // Listens to selection changes.
        Listbox::Listener* listener_;

        // The object that actually implements the table.
        NativeListboxWrapper* native_wrapper_;

        DISALLOW_COPY_AND_ASSIGN(Listbox);
    };

} //namespace view

#endif //__view_listbox_h__