
#include "gantt_main.h"

#include "base/at_exit.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "ui_base/resource/resource_bundle.h"
#include "ui_base/win/hwnd_util.h"
#include "view/focus/accelerator_handler.h"
#include "view/widget/widget.h"

#include "../wanui_res/resource.h"
#include "gantt_view.h"

GanttMain::GanttMain() : contents_(NULL) {}

GanttMain::~GanttMain() {}

bool GanttMain::CanResize() const
{
    return true;
}

bool GanttMain::CanMaximize() const
{
    return true;
}

std::wstring GanttMain::GetWindowTitle() const
{
    return std::wstring(L"iGantt");
}

view::View* GanttMain::GetContentsView()
{
    return contents_;
}

void GanttMain::WindowClosing()
{
    MessageLoopForUI::current()->Quit();
}

view::Widget* GanttMain::GetWidget()
{
    return contents_->GetWidget();
}

const view::Widget* GanttMain::GetWidget() const
{
    return contents_->GetWidget();
}

void GanttMain::Run()
{
    base::EnableTerminationOnHeapCorruption();

    base::AtExitManager exit_manager;

    FilePath res_dll;
    PathProvider(base::DIR_EXE, &res_dll);
    res_dll = res_dll.Append(L"wanui_res.dll");
    ui::ResourceBundle::InitSharedInstance(res_dll);

    MessageLoop main_message_loop(MessageLoop::TYPE_UI);

    DCHECK(contents_ == NULL) << "Run called more than once.";
    contents_ = new GanttView();

    view::Widget* window = view::Widget::CreateWindowWithBounds(this, gfx::Rect());
    ui::CenterAndSizeWindow(NULL, window->GetNativeWindow(),
        gfx::Size(800, 400), false);

    window->Show();
    view::AcceleratorHandler accelerator_handler;
    MessageLoopForUI::current()->Run(&accelerator_handler);

    ui::ResourceBundle::CleanupSharedInstance();
}