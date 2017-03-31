#ifndef WINDOW_INTERFACE_H
#define WINDOW_INTERFACE_H

#include "base/memory/singleton.h"
#include "base/message_loop.h"
#include "browser.h"
#include "browser_list.h"
#include "browser_window.h"
#include "tab_strip_model.h"
#include "tab_contents_wrapper.h"
#include "browser_view.h"
#include "toolbar_view.h"
#include "FsmInterface.h"


//#include "native_putty_common.h"
void fatalbox(const char *fmt, ...);


class WindowInterface
{
public:
	WindowInterface()
		: cmd_scatter_state_(CMD_DEFAULT)
	{}
	~WindowInterface(){}
	
	static WindowInterface* GetInstance(){
		return Singleton<WindowInterface>::get();
	}

	void createNewSession()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return ;
		}
		if (NULL != browser->AddBlankTab(true)){
			browser->window()->Show();
		}
	}

	void createNewSessionWithGlobalCfg()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return ;
		}
		if (NULL != browser->AddTabWithGlobalCfg(true)){
			browser->window()->Show();
		}
	}

	void dupCurSession()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return ;
		}
		browser->DuplicateCurrentTab();
		browser->window()->Show();
	}

	void selectNextTab()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return ;
		}
		browser->SelectNextTab();
	}

	void selectPreviousTab()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return ;
		}
		browser->SelectPreviousTab();
	}

	void moveTabNext()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return ;
		}
		browser->MoveTabNext();
	}

	void moveTabPrevious()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return ;
		}
		browser->MoveTabPrevious();
	}

	void selectTab(int index)
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return ;
		}
		browser->SelectNumberedTab(index);
	}

	void closeCurrentTab()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return ;
		}
		browser->CloseTab();
	}

	void reloadCurrentSession()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return ;
		}
		TabContents* tab = browser->GetSelectedTabContents();
		if (tab)
		{
			tab->do_restart();
		}
	}

	BrowserWindow* getBrowserView()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return NULL;
		}
		return browser->window();
	}

	HWND getNativeTopWnd()
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL)
			return NULL;
		BrowserWindow* bw = browser->window();
		if (bw == NULL)
			return NULL;
		return bw->GetNativeHandle();
	}

	enum{CMD_DEFAULT = 0, CMD_TO_ALL, CMD_TO_WITHIN_WINDOW, CMD_TO_ACTIVE_TAB};
	void setCmdScatterState(int state){
		if (state == cmd_scatter_state_) return;
		cmd_scatter_state_ = state;
		BrowserList::const_iterator it = BrowserList::begin();
		for (; it != BrowserList::end(); it++){
			((BrowserView*)(*it)->window())->toolbar()->UpdateButton() ;
		}
	}
	int getCmdScatterState(){return cmd_scatter_state_;}
	bool ifNeedCmdScat(){return CMD_DEFAULT != getCmdScatterState();}
	void cmdScat(int type, const char * buffer, int buflen, int interactive){
		if (CMD_TO_ALL == getCmdScatterState()){
			BrowserList::const_iterator it = BrowserList::begin();
			for (; it != BrowserList::end(); it++){
				TabStripModel* tabStripModel = (*it)->tabstrip_model();
				for (int i = 0; i < tabStripModel->count(); i++){
					TabContentsWrapper* tabContentsWrapper = tabStripModel->GetTabContentsAt(i);
					if (NULL == tabContentsWrapper) continue;
					TabContents* tab = tabContentsWrapper->tab_contents();
					tab->cmdScat(type, buffer, buflen, interactive);
				}
			}
		}
		else if (CMD_TO_WITHIN_WINDOW == getCmdScatterState()){
			Browser* browser = BrowserList::GetLastActive();
			TabStripModel* tabStripModel = browser->tabstrip_model();
			for (int i = 0; i < tabStripModel->count(); i++){
				TabContentsWrapper* tabContentsWrapper = tabStripModel->GetTabContentsAt(i);
				if (NULL == tabContentsWrapper) continue;
				TabContents* tab = tabContentsWrapper->tab_contents();
				tab->cmdScat(type, buffer, buflen, interactive);
			}
		}
		else if(CMD_TO_ACTIVE_TAB == getCmdScatterState()){
			BrowserList::const_iterator it = BrowserList::begin();
			for (; it != BrowserList::end(); it++){
				TabStripModel* tabStripModel = (*it)->tabstrip_model();
				TabContentsWrapper* tabContentsWrapper = tabStripModel->GetActiveTabContents();
				if (NULL == tabContentsWrapper) continue;
				TabContents* tab = tabContentsWrapper->tab_contents();
				tab->cmdScat(type, buffer, buflen, interactive);
			}
		}
	}

	void init_ui_msg_loop(){ ui_msg_loop_ = MessageLoopForUI::current(); }
	void process_in_msg_loop(FSM_FUNCTION<void(void)> func){
		if (ui_msg_loop_ == NULL){ return; }
		ui_msg_loop_->PostTask(new FsmTask(func));
	}

private:
	int cmd_scatter_state_;
	MessageLoopForUI* ui_msg_loop_;
	friend struct DefaultSingletonTraits<WindowInterface>;
	DISALLOW_COPY_AND_ASSIGN(WindowInterface);
};

#endif /* WINDOW_INTERFACE_H */
