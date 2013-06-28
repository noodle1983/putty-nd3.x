#include "putty_view.h"

namespace view{
	// static
    const char PuttyView::kViewClassName[] = "view/PuttyView";

	PuttyView::PuttyView(){
		set_background(Background::CreateStandardPanelBackground());
	}
    PuttyView::~PuttyView(){

	}

	void PuttyView::Layout(){

	}
}
