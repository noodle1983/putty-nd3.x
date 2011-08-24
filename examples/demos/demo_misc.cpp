
#include "demo_misc.h"

#include "ui_gfx/image/image.h"

#include "ui_base/resource/resource_bundle.h"

#include "view/controls/button/text_button.h"
#include "view/controls/focusable_border.h"
#include "view/controls/image_view.h"
#include "view/controls/link.h"
#include "view/controls/progress_bar.h"
#include "view/controls/scroll_view.h"
#include "view/controls/single_split_view.h"
#include "view/controls/throbber.h"
#include "view/layout/box_layout.h"
#include "view/layout/grid_layout.h"

#include "../wanui_res/resource.h"

DemoMisc::DemoMisc(DemoMain* main) : DemoBase(main) {}

DemoMisc::~DemoMisc() {}

std::wstring DemoMisc::GetDemoTitle()
{
    return std::wstring(L"Misc");
}

void DemoMisc::CreateDemoView(view::View* container)
{
    view::ImageView* image_view = new view::ImageView();
    image_view->SetImage(ui::ResourceBundle::GetSharedInstance().GetImageNamed(
        IDR_PRODUCT_LOGO_32));

    view::View* throbber_container = new view::View();
    throbber_container->SetLayoutManager(new view::BoxLayout(
        view::BoxLayout::kHorizontal, 0, 0, 10));
    checkmark_throbber_ = new view::CheckmarkThrobber();
    checkmark_throbber_->Start();
    checkmark_complete_ = new view::NativeTextButton(this, L"完成");
    throbber_container->AddChildView(checkmark_throbber_);
    throbber_container->AddChildView(checkmark_complete_);

    view::View* progress_container = new view::View();
    progress_container->SetLayoutManager(new view::BoxLayout(
        view::BoxLayout::kHorizontal, 0, 0, 10));
    progress_ = new view::ProgressBar();
    progress_->SetDisplayRange(0.0, 100.0);
    progress_del_ = new view::NativeTextButton(this, L"Del");
    progress_add_ = new view::NativeTextButton(this, L"Add");
    progress_container->AddChildView(progress_);
    progress_container->AddChildView(progress_del_);
    progress_container->AddChildView(progress_add_);

    view::View* view1 = new view::View();
    view1->set_border(new view::FocusableBorder());
    view1->SetLayoutManager(new view::BoxLayout(
        view::BoxLayout::kVertical, 0, 0, 10));
    view::CheckmarkThrobber* checkmark_throbber = new view::CheckmarkThrobber();
    checkmark_throbber->Start();
    view1->AddChildView(checkmark_throbber);

    view::View* scroll_container = new view::View();
    view::GridLayout* grid_layout = new view::GridLayout(scroll_container);
    scroll_container->SetLayoutManager(grid_layout);
    view::ColumnSet* column_set = grid_layout->AddColumnSet(0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL, 1,
        view::GridLayout::USE_PREF, 0, 0);
    grid_layout->StartRow(1, 0);

    view::ScrollView* scroll_view = new view::ScrollView();
    view::View* scroll_content = new view::View();
    scroll_content->SetBounds(0, 0, 100, 300);
    scroll_content->SetLayoutManager(new view::BoxLayout(
        view::BoxLayout::kVertical, 5, 5, 5));
    scroll_content->AddChildView(new view::Label(L"标签内容"));
    scroll_view->SetContents(scroll_content);
    scroll_view->set_border(new view::FocusableBorder());

    split_view_ = new view::SingleSplitView(view1,
        scroll_view, view::SingleSplitView::HORIZONTAL_SPLIT, NULL);
    grid_layout->AddView(split_view_);

    container->SetLayoutManager(new view::BoxLayout(
        view::BoxLayout::kVertical, 5, 5, 5));

    container->AddChildView(image_view);
    container->AddChildView(throbber_container);
    container->AddChildView(progress_container);
    container->AddChildView(scroll_container);
}

void DemoMisc::ButtonPressed(view::Button* sender,
                             const view::Event& event)
{
    if(checkmark_complete_ == sender)
    {
        checkmark_throbber_->Stop();
        checkmark_throbber_->SetChecked(true);
    }
    else if(progress_del_ == sender)
    {
        progress_->SetValue(progress_->current_value() - 10.0);
    }
    else if(progress_add_ == sender)
    {
        progress_->SetValue(progress_->current_value() + 10);
    }
}