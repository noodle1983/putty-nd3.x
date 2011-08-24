
#include "demo_textfield.h"

#include "base/stringprintf.h"

#include "view/layout/grid_layout.h"
#include "view/controls/label.h"
#include "view/controls/focusable_border.h"
#include "view/controls/richedit/rich_view.h"
#include "view/controls/textfield/textfield.h"

#include "demo_main.h"

DemoTextfiled::DemoTextfiled(DemoMain* main) : DemoBase(main) {}

DemoTextfiled::~DemoTextfiled() {}

std::wstring DemoTextfiled::GetDemoTitle()
{
    return std::wstring(L"Textfiled");
}

void DemoTextfiled::CreateDemoView(view::View* container)
{
    view::GridLayout* layout = new view::GridLayout(container);
    container->SetLayoutManager(layout);

    name_ = new view::Textfield();
    password_ = new view::Textfield(view::Textfield::STYLE_PASSWORD);
    multi_line_ = new view::Textfield(view::Textfield::STYLE_MULTILINE);
    richview_ = new view::RichView(ES_MULTILINE|WS_VSCROLL|ES_AUTOVSCROLL);
    richview_->set_border(new view::FocusableBorder());
    richview_->set_background(view::Background::CreateSolidBackground(195, 214, 238));
    name_->SetController(this);
    password_->SetController(this);

    view::ColumnSet* column_set = layout->AddColumnSet(0);
    column_set->AddColumn(view::GridLayout::LEADING, view::GridLayout::FILL,
        0.0f, view::GridLayout::USE_PREF, 0, 0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        1.0f, view::GridLayout::USE_PREF, 0, 0);
    layout->StartRow(0, 0);
    layout->AddView(new view::Label(L"Name:"));
    layout->AddView(name_);
    layout->StartRow(0, 0);
    layout->AddView(new view::Label(L"Password:"));
    layout->AddView(password_);

    column_set = layout->AddColumnSet(1);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        1.0f, view::GridLayout::USE_PREF, 0, 0);
    layout->StartRow(1, 1);
    layout->AddView(multi_line_);
    layout->StartRow(1, 1);
    layout->AddView(richview_);
}

void DemoTextfiled::ContentsChanged(view::Textfield* sender,
                                    const string16& new_contents)
{
    if(sender == name_)
    {
        PrintStatus(base::StringPrintf(L"Name [%ls]", new_contents.c_str()));
    }
    else if(sender == password_)
    {
        PrintStatus(base::StringPrintf(L"Password [%ls]",new_contents.c_str()));
    }
}

bool DemoTextfiled::HandleKeyEvent(view::Textfield* sender,
                                   const view::KeyEvent& key_event)
{
    return false;
}