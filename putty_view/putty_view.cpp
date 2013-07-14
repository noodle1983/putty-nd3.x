#include "putty_view.h"
#include "native_putty_controller.h"

#include "view/widget/widget.h"

namespace view{
	// static
    const char PuttyView::kViewClassName[] = "view/PuttyView";

	PuttyView::PuttyView()
	{
		Config cfg;
		do_defaults(NULL, &cfg);
		strcpy(cfg.session_name, "test");
		strcpy(cfg.host, "192.168.0.3");
		cfg.port = 22;
		cfg.protocol = PROT_SSH;
		puttyController_ = new NativePuttyController(&cfg, this);
		//SetVisible(false);
		set_focusable(true);
		//set_background(Background::CreateStandardPanelBackground());
	}
    PuttyView::~PuttyView(){
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
		RECT viewRect = ConvertRectToWidget(bounds()).ToRECT();
		puttyController_->setPagePos(&viewRect);
	}


	void PuttyView::Paint(gfx::Canvas* canvas){
		View::Paint(canvas);
	}
	
	bool PuttyView::OnKeyPressed(const KeyEvent& event)
	{
		const MSG& nativeEvent = event.native_event();
		return !puttyController_->on_key(puttyController_->getNativePage(), 
			nativeEvent.message, 
			nativeEvent.wParam,  
			nativeEvent.lParam);;
	}
	bool PuttyView::OnMousePressed(const MouseEvent& event)
	{
		const MSG& nativeEvent = event.native_event();
		puttyController_->on_button(puttyController_->getNativePage(), 
			nativeEvent.message, 
			nativeEvent.wParam,  
			nativeEvent.lParam);
		return true;
	}
    bool PuttyView::OnMouseDragged(const MouseEvent& event)
	{
		const MSG& nativeEvent = event.native_event();
		puttyController_->on_mouse_move(puttyController_->getNativePage(), 
			nativeEvent.message, 
			nativeEvent.wParam,  
			nativeEvent.lParam);
		return true;
	}
    void PuttyView::OnMouseReleased(const MouseEvent& event)
	{
		const MSG& nativeEvent = event.native_event();
		puttyController_->on_button(puttyController_->getNativePage(), 
			nativeEvent.message, 
			nativeEvent.wParam,  
			nativeEvent.lParam);;
	}

	void PuttyView::OnFocus()
	{
		::SetFocus(puttyController_->getNativePage());
        if(GetWidget())
        {
            GetWidget()->NotifyAccessibilityEvent(
                this, ui::AccessibilityTypes::EVENT_FOCUS, false);
        }
	}
}
