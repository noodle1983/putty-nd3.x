
#ifndef __demo_table_h__
#define __demo_table_h__

#pragma once

#include "SkBitmap.h"

#include "ui_base/models/table_model.h"
#include "ui_base/models/table_model_observer.h"
#include "view/controls/button/button.h"
#include "view/controls/table/table_view_observer.h"

#include "demo_base.h"

namespace view
{
    class Checkbox;
    class TableView2;
}

class DemoTable : public DemoBase, public ui::TableModel,
    public view::TableViewObserver,
    public view::ButtonListener
{
public:
    explicit DemoTable(DemoMain* main);
    virtual ~DemoTable();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from ui::TableModel:
    virtual int RowCount();
    virtual string16 GetText(int row, int column_id);
    virtual SkBitmap GetIcon(int row);
    virtual void SetObserver(ui::TableModelObserver* observer);

    // Overridden from view::TableViewObserver:
    virtual void OnSelectionChanged();
    virtual void OnDoubleClick();
    virtual void OnMiddleClick();
    virtual void OnKeyDown(ui::KeyboardCode virtual_keycode);
    virtual void OnTableViewDelete(view::TableView* table_view);
    virtual void OnTableView2Delete(view::TableView2* table_view);

    // Overridden from view::ButtonListener:
    virtual void ButtonPressed(view::Button* sender,
        const view::Event& event);

private:
    view::TableView2* table_view2_;

    view::Checkbox* column1_visible_checkbox_;

    SkBitmap icon1_;
    SkBitmap icon2_;

    DISALLOW_COPY_AND_ASSIGN(DemoTable);
};

#endif //__demo_table_h__