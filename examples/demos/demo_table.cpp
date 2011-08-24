
#include "demo_table.h"

#include "base/stringprintf.h"
#include "base/utf_string_conversions.h"

#include "SkCanvas.h"

#include "view/controls/button/checkbox.h"
#include "view/controls/table/table_view2.h"
#include "view/layout/grid_layout.h"

#include "demo_main.h"

DemoTable::DemoTable(DemoMain* main) : DemoBase(main) {}

DemoTable::~DemoTable() {}

std::wstring DemoTable::GetDemoTitle()
{
    return std::wstring(L"Table2");
}

void DemoTable::CreateDemoView(view::View* container)
{
    view::GridLayout* layout = new view::GridLayout(container);
    container->SetLayoutManager(layout);

    std::vector<ui::TableColumn> columns;
    columns.push_back(ui::TableColumn(0, ASCIIToUTF16("Fruit"),
        ui::TableColumn::LEFT, 100));
    columns.push_back(ui::TableColumn(1, ASCIIToUTF16("Color"),
        ui::TableColumn::CENTER, 100));
    columns.push_back(ui::TableColumn(2, ASCIIToUTF16("Origin"),
        ui::TableColumn::LEFT, 100));
    columns.push_back(ui::TableColumn(3, ASCIIToUTF16("Price"),
        ui::TableColumn::RIGHT, 100));
    const int options = (view::TableView2::SINGLE_SELECTION |
        view::TableView2::RESIZABLE_COLUMNS |
        view::TableView2::AUTOSIZE_COLUMNS |
        view::TableView2::HORIZONTAL_LINES |
        view::TableView2::VERTICAL_LINES);
    table_view2_ = new view::TableView2(this, columns, view::ICON_AND_TEXT, options);
    table_view2_->SetObserver(this);

    column1_visible_checkbox_ = new view::Checkbox(L"Fruit column visible");
    column1_visible_checkbox_->SetChecked(true);
    column1_visible_checkbox_->set_listener(this);

    icon1_.setConfig(SkBitmap::kARGB_8888_Config, 16, 16);
    icon1_.allocPixels();
    SkCanvas canvas1(icon1_);
    canvas1.drawColor(SK_ColorRED);

    icon2_.setConfig(SkBitmap::kARGB_8888_Config, 16, 16);
    icon2_.allocPixels();
    SkCanvas canvas2(icon2_);
    canvas2.drawColor(SK_ColorBLUE);

    view::ColumnSet* column_set = layout->AddColumnSet(0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL, 1,
        view::GridLayout::USE_PREF, 0, 0);
    layout->StartRow(1, 0);
    layout->AddView(table_view2_);

    column_set = layout->AddColumnSet(1);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        0.5f, view::GridLayout::USE_PREF, 0, 0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        0.5f, view::GridLayout::USE_PREF, 0, 0);

    layout->StartRow(0, 1);

    layout->AddView(column1_visible_checkbox_);
}

int DemoTable::RowCount()
{
    return 10;
}

string16 DemoTable::GetText(int row, int column_id)
{
    const char* const cells[5][4] =
    {
        { "Orange", "Orange", "South america", "$5" },
        { "Apple", "Green", "Canada", "$3" },
        { "Blue berries", "Blue", "Mexico", "$10.3" },
        { "Strawberries", "Red", "California", "$7" },
        { "Cantaloupe", "Orange", "South america", "$5" }
    };
    return ASCIIToUTF16(cells[row%5][column_id]);
}

SkBitmap DemoTable::GetIcon(int row)
{
    return row%2 ? icon1_ : icon2_;
}

void DemoTable::SetObserver(ui::TableModelObserver* observer) {}

void DemoTable::OnSelectionChanged()
{
    PrintStatus(base::StringPrintf(L"Selection changed: %d",
        table_view2_->GetFirstSelectedRow()));
}

void DemoTable::OnDoubleClick() {}

void DemoTable::OnMiddleClick() {}

void DemoTable::OnKeyDown(ui::KeyboardCode virtual_keycode) {}

void DemoTable::OnTableViewDelete(view::TableView* table_view) {}

void DemoTable::OnTableView2Delete(view::TableView2* table_view) {}

void DemoTable::ButtonPressed(view::Button* sender,
                              const view::Event& event)
{
    if(column1_visible_checkbox_ == sender)
    {
        table_view2_->SetColumnVisibility(0,
            column1_visible_checkbox_->checked());
    }
}