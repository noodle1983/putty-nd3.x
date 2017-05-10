#ifndef ONLINE_SESSION_MANAGER_H
#define ONLINE_SESSION_MANAGER_H

#include "../fsm/WinProcessor.h"
#include "AuthIdProtocol.h"
#include "../fsm/TcpServer.h"

#include<vector>
#include<string>
using namespace std;

class OnlineSessionManager :public Net::IProtocol
{
public:
	OnlineSessionManager();
	virtual ~OnlineSessionManager();

	int upload_sessions();

	void download_sessions();

	void handleInput(SocketConnectionPtr connection);
	virtual void handleClose(SocketConnectionPtr theConnection);
private:
	void handle_waiting_list();
	static void upload_file(string file);
	static void upload_file_done(bool is_success, string response);

private:
	vector<string> mWaitingList;
	int mHandlingIndex;

	string mRsp;
	TcpServer mTcpServer;
};

#define g_online_session_manager (DesignPattern::Singleton<OnlineSessionManager, 0>::instance())

#endif