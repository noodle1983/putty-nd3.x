#ifndef ONLINE_SESSION_MANAGER_H
#define ONLINE_SESSION_MANAGER_H

#include "../fsm/WinProcessor.h"

#include<vector>
#include<string>
using namespace std;

class OnlineSessionManager
{
public:
	OnlineSessionManager();
	virtual ~OnlineSessionManager();

	int upload_sessions();

	void download_sessions();

private:
	void handle_waiting_list();
	static void upload_file(string file);
	static void upload_file_done(bool is_success);

private:
	vector<string> mWaitingList;
	int mHandlingIndex;
};

#define g_online_session_manager (DesignPattern::Singleton<OnlineSessionManager, 0>::instance())

#endif