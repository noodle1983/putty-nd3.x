
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


#include "putty_callback.h"
#include "CmdLineHandler.h"

ChromeMain::ChromeMain() {}

ChromeMain::~ChromeMain() {}

void ChromeMain::Run()
{
    base::EnableTerminationOnHeapCorruption();

    FilePath res_dll;
    PathService::Get(base::DIR_EXE, &res_dll);
    res_dll = res_dll.Append(L"wanui_res.dll");
    ui::ResourceBundle::InitSharedInstance(res_dll);

    MessageLoop main_message_loop(MessageLoop::TYPE_UI);
	
	process_init();
	CmdLineHandler::GetInstance()->handleCmd();

    // Show Main Window...
    Browser* chrome = Browser::Create();

	TabContentsWrapper* tabContent = 
		CmdLineHandler::GetInstance()->isLeaderStartWithCmd() ? chrome->AddTabWithGlobalCfg(true)
			: chrome->AddBlankTab(true);
    if (tabContent){
		chrome->window()->Show();
		view::AcceleratorHandler accelerator_handler;
		MessageLoopForUI::current()->Run(&accelerator_handler);
	}
	process_fini();
    ui::ResourceBundle::CleanupSharedInstance();
}