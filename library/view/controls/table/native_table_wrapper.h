
#ifndef __view_native_table_wrapper_h__
#define __view_native_table_wrapper_h__

#pragma once

namespace gfx
{
    class Rect;
}

namespace ui
{
    struct TableColumn;
}

namespace view
{

    class TableView2;
    class View;

    // An interface implemented by an object that provides a platform-native
    // table.
    class NativeTableWrapper
    {
    public:
        // Returns the number of rows in the table.
        virtual int GetRowCount() const = 0;

        // Inserts/removes a column at the specified index.
        virtual void InsertColumn(const ui::TableColumn& column, int index) = 0;
        virtual void RemoveColumn(int index) = 0;

        // Returns the number of rows that are selected.
        virtual int GetSelectedRowCount() const = 0;

        // Returns the first row that is selected/focused in terms of the model.
        virtual int GetFirstSelectedRow() const = 0;
        virtual int GetFirstFocusedRow() const = 0;

        // Unselect all rows.
        virtual void ClearSelection() = 0;

        virtual void ClearRowFocus() = 0;

        virtual void SetSelectedState(int model_row, bool state) = 0;

        virtual void SetFocusState(int model_row, bool state) = 0;

        // Returns true if the row at the specified index is selected.
        virtual bool IsRowSelected(int model_row) const = 0;

        // Returns true if the row at the specified index has the focus.
        virtual bool IsRowFocused(int model_row) const = 0;

        // Retrieves the views::View that hosts the native control.
        virtual View* GetView() = 0;

        // Sets the focus to the table.
        virtual void SetFocus() = 0;

        // Returns a handle to the underlying native view for testing.
        virtual HWND GetTestingHandle() const = 0;

        // Gets/sets the columns width.
        virtual int GetColumnWidth(int column_index) const = 0;
        virtual void SetColumnWidth(int column_index, int width) = 0;

        // Called by the table view to indicate that some rows have changed, been
        // added or been removed.
        virtual void OnRowsChanged(int start, int length) = 0;
        virtual void OnRowsAdded(int start, int length) = 0;
        virtual void OnRowsRemoved(int start, int length) = 0;

        // Returns the bounds of the native table.
        virtual gfx::Rect GetBounds() const = 0;

        // Creates an appropriate NativeTableWrapper for the platform.
        static NativeTableWrapper* CreateNativeWrapper(TableView2* table);

    protected:
        virtual ~NativeTableWrapper() {}
    };

} //namespace view

#endif //__view_native_table_wrapper_h__