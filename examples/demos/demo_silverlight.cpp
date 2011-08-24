
#include "demo_silverlight.h"

#include "base/sys_string_conversions.h"
#include "view/controls/activex/silverlight_view.h"
#include "view/controls/button/text_button.h"
#include "view/layout/grid_layout.h"

#include "demo_main.h"

DemoSilverlight::DemoSilverlight(DemoMain* main) : DemoBase(main),
silverlight_view1_(NULL), silverlight_view2_(NULL), button_(NULL) {}

DemoSilverlight::~DemoSilverlight() {}

std::wstring DemoSilverlight::GetDemoTitle()
{
    return std::wstring(L"Silverlight");
}

void DemoSilverlight::CreateDemoView(view::View* container)
{
    view::GridLayout* layout = new view::GridLayout(container);
    container->SetLayoutManager(layout);

    view::ColumnSet* column_set = layout->AddColumnSet(0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        1.0f, view::GridLayout::USE_PREF, 0, 0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        1.0f, view::GridLayout::USE_PREF, 0, 0);

    view::ColumnSet* column_set1 = layout->AddColumnSet(1);
    column_set1->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        1.0f, view::GridLayout::USE_PREF, 0, 0);

    layout->StartRow(1.0f, 0);
    silverlight_view1_ = new view::SilverlightView();
    layout->AddView(silverlight_view1_);
    silverlight_view2_ = new view::SilverlightView();
    layout->AddView(silverlight_view2_);

    layout->StartRow(0.0f, 1);
    button_ = new view::NativeTextButton(this, L"¼ÓÔØSilverlightÎÄ¼þ");
    layout->AddView(button_);
}

void DemoSilverlight::ButtonPressed(view::Button* sender,
                                    const view::Event& event)
{
    if(button_ == sender)
    {
        silverlight_view1_->Play(L"http://www.andybeaulieu.com/silverlight/2.0/sortthefoobars/ClientBin/SortTheFoobars.xap");
        silverlight_view1_->Play(L"http://www.andybeaulieu.com/silverlight/2.0/sortthefoobars/ClientBin/SortTheFoobars.xap");
    }
}