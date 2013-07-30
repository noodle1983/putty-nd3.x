#include "putty_view.h"
#include "native_putty_controller.h"

#include "view/widget/widget.h"
#include "putty_global_config.h"

#include "terminal.h"
#include "storage.h"
extern int is_session_log_enabled(void *handle);
extern void log_restart(void *handle, Config *cfg);
extern void log_stop(void *handle, Config *cfg);

namespace view{
	// static
    const char PuttyView::kViewClassName[] = "view/PuttyView";

	PuttyView::PuttyView()
	{
		extern Config cfg;
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

	string16& PuttyView::getWinTitle(){
		return puttyController_->disName;
	}

	HWND PuttyView::getNativeView(){
		return puttyController_->getNativePage();
	}

	bool PuttyView::isLoading(){
		return puttyController_->isLoading();
	}
	bool PuttyView::isDisconnected(){
		return puttyController_->isDisconnected();
	}
	void PuttyView::dupCfg2Global(){
		extern Config cfg;
		cfg = puttyController_->cfg;
	}

	void PuttyView::do_copy()
	{

	}

	void PuttyView::do_paste()
	{
		puttyController_->request_paste();
	}

	void PuttyView::do_restart()
	{
		puttyController_->restartBackend();
	}

	void PuttyView::do_reconfig()
	{
		puttyController_->on_reconfig();
	}

	void PuttyView::do_copyAll()
	{
		return term_copyall(puttyController_->term);
	}

	void PuttyView::do_clearScrollbar()
	{
		return term_clrsb(puttyController_->term);
	}

	void PuttyView::do_log(bool isPressed)
	{
		bool isStarted = is_session_log_enabled(puttyController_->logctx)!= 0;
		if (isStarted == isPressed)
			return;

		if (!isStarted)
        {
            log_restart(puttyController_->logctx, &puttyController_->cfg);
        }
        else
        {
    		/* Pass new config data to the logging module */
    		log_stop(puttyController_->logctx, &puttyController_->cfg);
        }
	}

	void PuttyView::do_shortcutEnabler(bool isPressed)
	{
		//puttyController_->cfg.is_enable_shortcut = isPressed;
		PuttyGlobalConfig::GetInstance()->setShotcutKeyEnabled(isPressed);
	}

	bool PuttyView::isLogStarted()
	{
		if (puttyController_->page_ == NULL)
			return false;
		return is_session_log_enabled(puttyController_->logctx)!= 0;
	}

	bool PuttyView::isShortcutEnabled()
	{
		return PuttyGlobalConfig::GetInstance()->isShotcutKeyEnabled();
	}

	void PuttyView::searchNext(const string16& str)
	{
		return term_find(puttyController_->term, str.c_str(), 0);
	}

	void PuttyView::searchPrevious(const string16& str)
	{
		return term_find(puttyController_->term, str.c_str(), 1);
	}

	void PuttyView::resetSearch() 
	{
		return term_free_hits(puttyController_->term);
	}

	void PuttyView::setFocus()
	{
		SetFocus(puttyController_->getNativePage());;
	}


}
