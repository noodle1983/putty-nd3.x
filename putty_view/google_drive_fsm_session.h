#ifndef GOOGLE_DRIVE_FSM_SESSION_H
#define GOOGLE_DRIVE_FSM_SESSION_H

#include "../fsm/WinProcessor.h"
#include "../fsm/TcpServer.h"
#include "../fsm/Protocol.h"

#include "Fsm.h"
#include "Action.h"
#include "State.h"
#include "Session.h"
#include <vector>
#include <string>
#include <map>
#include <set>
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
		GET_SESSION_FOLDER,
		GET_EXIST_SESSIONS_ID,
		CREATE_SESSION_FOLDER,
		CHECK_ACTION,
		GET_REST_SESSIONS_ID,
		UPLOAD_SESSION,
		UPLOAD_DONE,
		DOWNLOAD_SESSION,
		DOWNLOAD_DONE,
		DELETE_SESSIONS,
		DELETE_DONE,
	};
	enum MyEvent
	{
		NETWORK_INPUT_EVT = 0,
		NEXT_EVT,
		CREATE_SESSION_FOLDER_EVT,
		UPLOAD_EVT,
		DOWNLOAD_EVT,
		DELETE_EVT,
		GET_REST_SESSIONS_ID_EVT,
		DONE_EVT,
		HTTP_SUCCESS_EVT,
		HTTP_FAILED_EVT,
	};

	static Fsm::FiniteStateMachine* getZmodemFsm();
	static bool not_to_upload(const char* session_name);

	GoogleDriveFsmSession();
	virtual ~GoogleDriveFsmSession();

	void LoadRemoteFile();
	void startUpload();
	void startDownload();

	map<string, string>& get_session_id_map(){ return mExistSessionsId; }
	void clear_in_all_list(const string& session){
		mUploadList.erase(session);
		mDeleteList.erase(session);
		mDownloadList.erase(session);
	}

private:
	//fsm
	void startProgressDlg();
	void stopProgressDlg();
	void updateProgressDlg(const string& title, const string& desc, int completed, int total);

	void initAll(); 
	void getAuthCode();
	void handleAuthCodeInput();
	void getAccessToken();
	void parseAccessToken();
	void getSessionFolder();
	void parseSessionFolderInfo();
	void createSessionFolder();
	void parseCreateSessionFolderInfo();
	void getExistSessionsId();
	void parseSessionsId();
	void checkAction();
	void uploadSession();
	void parseUploadSession();
	void downloadSession();
	void parseDownloadSession();
	void downloadDone();

	//protocol
	void handleInput(Net::SocketConnectionPtr connection);
	virtual void handleClose(Net::SocketConnectionPtr theConnection);

	void bgHttpRequest(const char* const method);
	void handleHttpRsp();
	void resetHttpData();
	static size_t query_auth_write_cb(void *_ptr, size_t _size, size_t _nmemb, void *_data);
	void update_ui_progress_for_http_request();
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
	string mAccessTokenHeader;
	bool mIsUpload;

	IProgressDialog * mProgressDlg;

	base::Lock mHttpLock;
	string mHttpUrl;
	string mPostData;
	vector<string> mHttpHeaders;
	string mHttpRsp;
	int mHttpResult;
	string mHttpProxy;
	int mHttpProxyType;

	string mSessionFolderId;
	map<string, string> mExistSessionsId;

public:
	map<string, string> mDownloadList;
	set<string> mDeleteList;
	map<string, string> mUploadList;
};

#define g_google_drive_fsm_session (DesignPattern::Singleton<GoogleDriveFsmSession, 0>::instance())

#endif