
#include "demo_list.h"

#include "view/layout/grid_layout.h"

#include "demo_main.h"

namespace
{
    static const string16 items[] =
    {
        L"Ïã½¶",
        L"ÆÏÌÑ",
        L"Æ»¹û",
        L"éÙ×Ó",
        L"ÌÒ×Ó"
    };
}

DemoList::DemoList(DemoMain* main) : DemoBase(main) {}

DemoList::~DemoList() {}

std::wstring DemoList::GetDemoTitle()
{
    return std::wstring(L"Listbox & ComboBox");
}

void DemoList::CreateDemoView(view::View* container)
{
    view::GridLayout* layout = new view::GridLayout(container);
    container->SetLayoutManager(layout);

    combobox_ = new view::Combobox(this);
    combobox_->set_listener(this);

    listbox_ = new view::Listbox(this);
    listbox_->set_listener(this);

    view::ColumnSet* column_set = layout->AddColumnSet(0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL, 1,
        view::GridLayout::USE_PREF, 0, 0);
    layout->StartRow(0, 0);
    layout->AddView(combobox_);

    layout->StartRow(1, 0);
    layout->AddView(listbox_);
}

int DemoList::GetItemCount()
{
    return arraysize(items);
}

string16 DemoList::GetItemAt(int index)
{
    return items[index];
}

void DemoList::ItemChanged(view::Combobox* combo_box,
                           int prev_index, int new_index)
{
    PrintStatus(combo_box->model()->GetItemAt(new_index));
}

void DemoList::SelectionChanged(view::Listbox* sender)
{
    PrintStatus(sender->model()->GetItemAt(
        sender->SelectedRow()));
}