#ifndef GOOGLE_DRIVE_FSM_SESSION_H
#define GOOGLE_DRIVE_FSM_SESSION_H

#include "../fsm/WinProcessor.h"
#include "../fsm/TcpServer.h"
#include "../fsm/Protocol.h"

#include "Fsm.h"
#include "Action.h"
#include "State.h"
#include "Session.h"
#include<vector>
#include<string>
using namespace std;

class GoogleDriveFsmSession :public Fsm::Session, Net::IProtocol
{
public:
	enum MyState
	{
		IDLE_STATE = 1,
		GET_AUTH_CODE_STATE,
		GET_ACCESS_TOKEN_STATE,
		GET_GET_SESSION_FOLDER,
	};
	enum MyEvent
	{
		NETWORK_INPUT_EVT = 0,
		NEXT_EVT
	};

	static Fsm::FiniteStateMachine* getZmodemFsm();

	GoogleDriveFsmSession();
	virtual ~GoogleDriveFsmSession();
private:
	//fsm
	void initAll(); 
	void getAuthCode();
	void handleAuthCodeInput();
	void getAccessToken();

	//protocol
	void handleInput(Net::SocketConnectionPtr connection);
	virtual void handleClose(Net::SocketConnectionPtr theConnection);
	
	void parse_auth_code(string query_string);
	void on_get_access_token(string& rsp);

private:
	static base::Lock fsmLock_;
	static std::auto_ptr<Fsm::FiniteStateMachine> fsm_;

	vector<string> mWaitingList;
	int mHandlingIndex;

	string mRsp;
	Net::TcpServer mTcpServer;

	string mState;
	string mCodeVerifier;
	string mRedirectUrl;
	string mCodeChallenge;
	string mAuthCodeInput;
	string mAuthCode;
};

#define g_google_drive_fsm_session (DesignPattern::Singleton<GoogleDriveFsmSession, 0>::instance())

#endif