
#include "demo_bubble.h"

#include "view/controls/button/text_button.h"
#include "view/controls/label.h"
#include "view/layout/box_layout.h"
#include "view/layout/grid_layout.h"

#include "demo_main.h"

#include "../bubble/bubble.h"

DemoBubble::DemoBubble(DemoMain* main) : DemoBase(main),
button1_(NULL), button2_(NULL), button3_(NULL),
button4_(NULL), button5_(NULL) {}

DemoBubble::~DemoBubble() {}

std::wstring DemoBubble::GetDemoTitle()
{
    return std::wstring(L"Bubble");
}

void DemoBubble::CreateDemoView(view::View* container)
{
    view::GridLayout* layout = new view::GridLayout(container);
    container->SetLayoutManager(layout);

    button1_ = new view::TextButton(this, L"显示Bubble");
    button2_ = new view::TextButton(this, L"显示Bubble");
    button3_ = new view::TextButton(this, L"显示Bubble");
    button4_ = new view::TextButton(this, L"显示Bubble");
    button5_ = new view::TextButton(this, L"显示Bubble");

    view::ColumnSet* column_set = layout->AddColumnSet(0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        1.0f, view::GridLayout::USE_PREF, 0, 0);
    view::ColumnSet* column_set1 = layout->AddColumnSet(1);
    column_set1->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        0.0f, view::GridLayout::USE_PREF, 0, 0);
    column_set1->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        1.0f, view::GridLayout::USE_PREF, 0, 0);
    column_set1->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
        0.0f, view::GridLayout::USE_PREF, 0, 0);
    
    layout->StartRow(0.0f, 0);
    layout->AddView(button1_);

    layout->StartRow(1.0f, 1);
    layout->AddView(button2_);
    layout->AddView(button3_);
    layout->AddView(button4_);

    layout->StartRow(0.0f, 0);
    layout->AddView(button5_);
}

void DemoBubble::ButtonPressed(view::Button* sender,
                               const view::Event& event)
{
    BubbleBorder::ArrowLocation arrow_location = BubbleBorder::FLOAT;
    if(button1_ == sender)
    {
        arrow_location = BubbleBorder::TOP_LEFT;
    }
    else if(button2_ == sender)
    {
        arrow_location = BubbleBorder::LEFT_TOP;
    }
    else if(button4_ == sender)
    {
        arrow_location = BubbleBorder::RIGHT_TOP;
    }
    else if(button5_ == sender)
    {
        arrow_location = BubbleBorder::BOTTOM_LEFT;
    }

    view::View* content = new view::View;
    content->SetLayoutManager(new view::BoxLayout(
        view::BoxLayout::kVertical, 0, 0, 0));
    content->AddChildView(new view::Label(L"这是一个绚丽的弹出式气泡窗口"));
    content->AddChildView(new view::Label(L"是一个layered窗口+一个普通的popup窗口"));

    gfx::Point point;
    view::View::ConvertPointToScreen(sender, &point);
    gfx::Rect rect(point, sender->bounds().size());
    Bubble::Show(sender->GetWidget(), rect, arrow_location, content, NULL);
}