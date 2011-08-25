
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

#include "../status_tray/status_icon.h"
#include "../status_tray/status_tray.h"
#include "../wanui_res/resource.h"

#include "demo_activex.h"
#include "demo_bubble.h"
#include "demo_button.h"
#include "demo_html.h"
#include "demo_label.h"
#include "demo_list.h"
#include "demo_misc.h"
#include "demo_silverlight.h"
#include "demo_table.h"
#include "demo_textfield.h"
#include "demo_webbrowser.h"

DemoMain::DemoMain() : contents_(NULL), status_label_(NULL),
main_status_icon_(NULL) {}

DemoMain::~DemoMain() {}

void DemoMain::DisplayBalloon()
{
    if(!main_status_icon_)
    {
        return;
    }

    main_status_icon_->DisplayBalloon(L"演示气泡",
        L"您点击了链接文本!\nwlwlxj@gmail.com");
}

void DemoMain::SetTrayIconContextMenu(ui::MenuModel* menu)
{
    if(!main_status_icon_)
    {
        return;
    }

    main_status_icon_->SetContextMenu(menu);
}

bool DemoMain::CanResize() const
{
    return true;
}

std::wstring DemoMain::GetWindowTitle() const
{
    return std::wstring(L"View Demo");
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
    PathProvider(base::DIR_EXE, &res_dll);
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

    DCHECK(!status_tray_.get());
    status_tray_.reset(StatusTray::Create());
    main_status_icon_ = status_tray_->CreateStatusIcon();
    main_status_icon_->SetToolTip(L"View Demo\nwlwlxj@gmail.com");
    main_status_icon_->SetImage(ui::ResourceBundle::GetSharedInstance().GetImageNamed(
        IDR_PRODUCT_LOGO_16));

    DemoLabel demo_label(this);
    DemoButton demo_button(this);
    DemoMisc demo_misc(this);
    DemoTable demo_table(this);
    DemoList demo_list(this);
    DemoTextfiled demo_textfiled(this);
    DemoBubble demo_bubble(this);
    DemoActiveX demo_activex(this);
    DemoWebBrowser demo_webbrowser(this);
    DemoHtml demo_html(this);
    DemoSilverlight demo_silverlight(this);
    tabbed_pane->AddTab(demo_label.GetDemoTitle(), demo_label.GetDemoView());
    tabbed_pane->AddTab(demo_button.GetDemoTitle(), demo_button.GetDemoView());
    tabbed_pane->AddTab(demo_misc.GetDemoTitle(), demo_misc.GetDemoView());
    tabbed_pane->AddTab(demo_table.GetDemoTitle(), demo_table.GetDemoView());
    tabbed_pane->AddTab(demo_list.GetDemoTitle(), demo_list.GetDemoView());
    tabbed_pane->AddTab(demo_textfiled.GetDemoTitle(), demo_textfiled.GetDemoView());
    tabbed_pane->AddTab(demo_bubble.GetDemoTitle(), demo_bubble.GetDemoView());
    tabbed_pane->AddTab(demo_activex.GetDemoTitle(), demo_activex.GetDemoView());
    tabbed_pane->AddTab(demo_webbrowser.GetDemoTitle(), demo_webbrowser.GetDemoView());
    tabbed_pane->AddTab(demo_html.GetDemoTitle(), demo_html.GetDemoView());
    tabbed_pane->AddTab(demo_silverlight.GetDemoTitle(), demo_silverlight.GetDemoView());

    window->Show();
    view::AcceleratorHandler accelerator_handler;
    MessageLoopForUI::current()->Run(&accelerator_handler);

    ui::ResourceBundle::CleanupSharedInstance();
}