
#ifndef __view_table_view_observer_h__
#define __view_table_view_observer_h__

#pragma once

#include "ui_base/keycodes/keyboard_codes_win.h"

namespace view
{

    class TableView;
    class TableView2;

    // TableViewObserver is notified about the TableView selection.
    class TableViewObserver
    {
    public:
        virtual ~TableViewObserver() {}

        // Invoked when the selection changes.
        virtual void OnSelectionChanged() = 0;

        // Optional method invoked when the user double clicks on the table.
        virtual void OnDoubleClick() {}

        // Optional method invoked when the user middle clicks on the table.
        virtual void OnMiddleClick() {}

        // Optional method invoked when the user hits a key with the table in focus.
        virtual void OnKeyDown(ui::KeyboardCode virtual_keycode) {}

        // Invoked when the user presses the delete key.
        virtual void OnTableViewDelete(TableView* table_view) {}

        // Invoked when the user presses the delete key.
        virtual void OnTableView2Delete(TableView2* table_view) {}
    };

} //namespace view

#endif //__view_table_view_observer_h__