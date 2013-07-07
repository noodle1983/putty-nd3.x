#include "putty_view.h"
#include "native_putty_controller.h"

namespace view{
	// static
    const char PuttyView::kViewClassName[] = "view/PuttyView";

	PuttyView::PuttyView()
		:puttyController_(new NativePuttyController)
	{
		Config cfg;
		do_defaults(NULL, &cfg);
		strcpy(cfg.session_name, "test");
		strcpy(cfg.host, "183.62.9.76");
		cfg.port = 22;
		puttyController_->init(&cfg, this);
		SetVisible(false);
		set_background(Background::CreateStandardPanelBackground());
	}
    PuttyView::~PuttyView(){
		puttyController_->fini();
		delete puttyController_;
	}

	void PuttyView::ViewHierarchyChanged(bool is_add, View* parent, View* child)
	{
		puttyController_->parentChanged(is_add ? parent : NULL);
	}

        // When SetVisible() changes the visibility of a view, this method is
        // invoked for that view as well as all the children recursively.
    void PuttyView::VisibilityChanged(View* starting_from, bool is_visible)
	{
		if (is_visible){
			puttyController_->showPage();
		}else{
			puttyController_->hidePage();
		}
	}

	void PuttyView::Layout(){

	}


	void PuttyView::Paint(gfx::Canvas* canvas){
		View::Paint(canvas);
	}
}
