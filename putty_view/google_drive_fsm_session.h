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
struct IProgressDialog;

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

	void startUpload();
	void startDownload();
private:
	//fsm
	void startProgressDlg();
	void stopProgressDlg();
	void updateProgressDlg(const string& title, const string& desc);

	void initAll(); 
	void getAuthCode();
	void handleAuthCodeInput();
	void getAccessToken();

	//protocol
	void handleInput(Net::SocketConnectionPtr connection);
	virtual void handleClose(Net::SocketConnectionPtr theConnection);
	
	// run in bg
	void bgGetAccessToken(string authCode, string codeVerifier, string redirectUrl);
	void handleAccessToken(string accessToken);

private:
	static base::Lock fsmLock_;
	static std::auto_ptr<Fsm::FiniteStateMachine> fsm_;

	string mRsp;
	Net::TcpServer mTcpServer;

	string mState;
	string mCodeVerifier;
	string mRedirectUrl;
	string mCodeChallenge;
	string mAuthCodeInput;
	string mAuthCode;
	string mAccessToken;
	bool mIsUpload;

	IProgressDialog * mProgressDlg;

	vector<string> mWaitingList;
	int mHandlingIndex;
};

#define g_google_drive_fsm_session (DesignPattern::Singleton<GoogleDriveFsmSession, 0>::instance())

#endif