
#include "demo_html.h"

#include "base/sys_string_conversions.h"
#include "view/controls/activex/html_view.h"
#include "view/controls/button/text_button.h"
#include "view/controls/textfield/textfield.h"
#include "view/layout/grid_layout.h"

#include "demo_main.h"

DemoHtml::DemoHtml(DemoMain* main) : DemoBase(main),
html_view_(NULL), html_(NULL), button_(NULL) {}

DemoHtml::~DemoHtml() {}

std::wstring DemoHtml::GetDemoTitle()
{
    return std::wstring(L"Html Document");
}

void DemoHtml::CreateDemoView(view::View* container)
{
    view::GridLayout* layout = new view::GridLayout(container);
    container->SetLayoutManager(layout);

    view::ColumnSet* column_set = layout->AddColumnSet(0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        1.0f, view::GridLayout::USE_PREF, 0, 0);

    view::ColumnSet* column_set1 = layout->AddColumnSet(1);
    column_set1->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        1.0f, view::GridLayout::USE_PREF, 0, 0);
    column_set1->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        0.0f, view::GridLayout::USE_PREF, 0, 0);

    layout->StartRow(1.0f, 0);
    html_view_ = new view::HtmlView();
    layout->AddView(html_view_);

    layout->StartRow(0.0f, 1);
    html_ = new view::Textfield();
    html_->SetText(L"<hr><p>I'm ÍòĞÇĞÇ</p><input/>");
    layout->AddView(html_);
    button_ = new view::NativeTextButton(this, L"äÖÈ¾");
    layout->AddView(button_);
}

void DemoHtml::ButtonPressed(view::Button* sender,
                             const view::Event& event)
{
    if(button_ == sender)
    {
        html_view_->SetHtml(base::SysWideToNativeMB(html_->text()));
    }
}