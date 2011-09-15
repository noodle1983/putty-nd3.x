
#include "demo_main.h"

#include "base/at_exit.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/stringprintf.h"
#include "ui_gfx/image/image.h"
#include "ui_base/resource/resource_bundle.h"
#include "view/controls/label.h"
#include "view/controls/tabbed_pane/tabbed_pane.h"
#include "view/focus/accelerator_handler.h"
#include "view/layout/grid_layout.h"
#include "view/widget/widget.h"

#include "../wanui_res/resource.h"

#include "demo_download.h"

DemoMain::DemoMain() : contents_(NULL), status_label_(NULL) {}

DemoMain::~DemoMain() {}

bool DemoMain::CanResize() const
{
    return true;
}

std::wstring DemoMain::GetWindowTitle() const
{
    return std::wstring(L"NetBase Test");
}

view::View* DemoMain::GetContentsView()
{
    return contents_;
}

void DemoMain::WindowClosing()
{
    MessageLoopForUI::current()->Quit();
}

view::Widget* DemoMain::GetWidget()
{
    return contents_->GetWidget();
}

const view::Widget* DemoMain::GetWidget() const
{
    return contents_->GetWidget();
}

void DemoMain::TabSelectedAt(int index)
{
    SetStatus(base::StringPrintf(L"Select tab: %d", index));
}

void DemoMain::SetStatus(const std::wstring& status)
{
    status_label_->SetText(status);
}

void DemoMain::Run()
{
    base::EnableTerminationOnHeapCorruption();

    base::AtExitManager exit_manager;

    FilePath res_dll;
    PathService::Get(base::DIR_EXE, &res_dll);
    res_dll = res_dll.Append(L"wanui_res.dll");
    ui::ResourceBundle::InitSharedInstance(res_dll);

    MessageLoop main_message_loop(MessageLoop::TYPE_UI);

    DCHECK(contents_ == NULL) << "Run called more than once.";
    contents_ = new view::View();
    contents_->set_background(view::Background::CreateStandardPanelBackground());
    view::GridLayout* layout = new view::GridLayout(contents_);
    contents_->SetLayoutManager(layout);

    view::ColumnSet* column_set = layout->AddColumnSet(0);
    column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL, 1,
        view::GridLayout::USE_PREF, 0, 0);

    view::TabbedPane* tabbed_pane = new view::TabbedPane();
    tabbed_pane->set_listener(this);
    status_label_ = new view::Label();

    layout->StartRow(1, 0);
    layout->AddView(tabbed_pane);
    layout->StartRow(0, 0);
    layout->AddView(status_label_);

    view::Widget* window = view::Widget::CreateWindowWithBounds(this,
        gfx::Rect(0, 0, 800, 400));

    DemoDownload demo_download(this);
    tabbed_pane->AddTab(demo_download.GetDemoTitle(), demo_download.GetDemoView());

    window->Show();
    view::AcceleratorHandler accelerator_handler;
    MessageLoopForUI::current()->Run(&accelerator_handler);

    ui::ResourceBundle::CleanupSharedInstance();
}