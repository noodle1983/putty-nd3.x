
#include "demo_webbrowser.h"

#include "view/controls/activex/webbrowser_view.h"
#include "view/controls/button/text_button.h"
#include "view/controls/textfield/textfield.h"
#include "view/layout/grid_layout.h"

#include "demo_main.h"

DemoWebBrowser::DemoWebBrowser(DemoMain* main) : DemoBase(main),
web_view_(NULL), web_address_(NULL), button_(NULL) {}

DemoWebBrowser::~DemoWebBrowser() {}

std::wstring DemoWebBrowser::GetDemoTitle()
{
    return std::wstring(L"WebBrowser");
}

void DemoWebBrowser::CreateDemoView(view::View* container)
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
    web_view_ = new view::WebBrowserView();
    layout->AddView(web_view_);

    layout->StartRow(0.0f, 1);
    web_address_ = new view::Textfield();
    web_address_->SetText(L"http://www.cppblog.com/wlwlxj");
    layout->AddView(web_address_);
    button_ = new view::NativeTextButton(this, L"µ¼º½");
    layout->AddView(button_);
}

void DemoWebBrowser::ButtonPressed(view::Button* sender,
                                   const view::Event& event)
{
    if(button_ == sender)
    {
        web_view_->Navigate(web_address_->text());
    }
}