
#include "chrome_main.h"

#include "base/at_exit.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/process_util.h"

#include "ui_base/resource/resource_bundle.h"

#include "view/focus/accelerator_handler.h"

#include "browser.h"
#include "browser_navigator.h"
#include "browser_window.h"
#include "tab_contents.h"
#include "tab_contents_view.h"
#include "tab_strip_model.h"

ChromeMain::ChromeMain() {}

ChromeMain::~ChromeMain() {}

void ChromeMain::Run()
{
    base::EnableTerminationOnHeapCorruption();

    base::AtExitManager exit_manager;

    FilePath res_dll;
    PathService::Get(base::DIR_EXE, &res_dll);
    res_dll = res_dll.Append(L"wanui_res.dll");
    ui::ResourceBundle::InitSharedInstance(res_dll);

    MessageLoop main_message_loop(MessageLoop::TYPE_UI);

    // Show Main Window...
    Browser* chrome = Browser::Create();
    if (chrome->AddBlankTab(true)){

		chrome->window()->Show();

		view::AcceleratorHandler accelerator_handler;
		MessageLoopForUI::current()->Run(&accelerator_handler);
	}
    ui::ResourceBundle::CleanupSharedInstance();
}