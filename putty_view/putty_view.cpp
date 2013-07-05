#include "putty_view.h"
#include "native_putty_controller.h"

namespace view{
	// static
    const char PuttyView::kViewClassName[] = "view/PuttyView";

	PuttyView::PuttyView()
		:puttyController_(new NativePuttyController)
	{
		Config cfg;
		strcpy(cfg.session_name, "test");
		strcpy(cfg.host, "183.62.9.76");
		cfg.port = 22;
		puttyController_->init(&cfg, this);
		set_background(Background::CreateStandardPanelBackground());
	}
    PuttyView::~PuttyView(){
		puttyController_->fini();
		delete puttyController_;
	}

	void PuttyView::Layout(){

	}


	void PuttyView::Paint(gfx::Canvas* canvas){
		View::Paint(canvas);
	}
}
