#ifndef ONLINE_SESSION_MANAGER_H
#define ONLINE_SESSION_MANAGER_H

#include "../fsm/WinProcessor.h"
#include "../fsm/TcpServer.h"
#include "../fsm/Protocol.h"

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

	void handleInput(Net::SocketConnectionPtr connection);
	virtual void handleClose(Net::SocketConnectionPtr theConnection);
private:
	void handle_waiting_list();
	static void upload_file(string file);
	static void upload_file_done(bool is_success, string response);

private:
	vector<string> mWaitingList;
	int mHandlingIndex;

	string mRsp;
	Net::TcpServer mTcpServer;
};

#define g_online_session_manager (DesignPattern::Singleton<OnlineSessionManager, 0>::instance())

#endif