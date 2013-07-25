#ifndef WINDOW_INTERFACE_H
#define WINDOW_INTERFACE_H

#include "base/memory/singleton.h"
#include "browser.h"
#include "browser_list.h"
#include "browser_window.h"
#include "native_putty_common.h"

class WindowInterface
{
public:
	WindowInterface()
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
		browser->AddBlankTab(true);
		browser->window()->Show();
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

	void selectTab(int index)
	{
		Browser* browser = BrowserList::GetLastActive();
		if (browser == NULL){
			fatalbox("%s", "last ative window is not found");
			return ;
		}
		browser->SelectNumberedTab(index);
	}

private:
	
	friend struct DefaultSingletonTraits<WindowInterface>;
	DISALLOW_COPY_AND_ASSIGN(WindowInterface);
};

#endif /* WINDOW_INTERFACE_H */
