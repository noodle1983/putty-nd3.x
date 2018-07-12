#include "google_drive_fsm_session.h"
#include "putty.h"
#include "storage.h"
#include "../adb/AdbManager.h"
#include <Windows.h>
#include <Winhttp.h>
#include "atlconv.h" 
#include <algorithm>
#include <iostream>
#include <string>
#include "../base/rand_util.h"
#undef min
#include "../base/algorithm/base64/base64.h"
#include "base/string_split.h"
extern "C"{
#include <curl/curl.h>
}
#include "../fsm/SocketConnection.h"
#include "../rapidjson/document.h"

#include <Shlobj.h>

using namespace Net;
using namespace rapidjson;

#include <shellapi.h>
struct ssh_hash {
	void *(*init)(void); /* also allocates context */
	void *(*copy)(const void *);
	void(*bytes)(void *, const void *, int);
	void(*final)(void *, unsigned char *); /* also frees context */
	void(*free)(void *);
	int hlen; /* output length in bytes */
	const char *text_name;
};

const char* const client_id = "924120620403-5qdo3gbs7eleo9a2v0ug039vsk758vln.apps.googleusercontent.com";
const char* const client_secret = "Ee6ZYZH9rq-xE2y6kyM4evoi";
base::Lock GoogleDriveFsmSession::fsmLock_;
std::auto_ptr<Fsm::FiniteStateMachine> GoogleDriveFsmSession::fsm_;
extern void set_progress_bar(const std::string& msg, int progress);

void get_remote_file()
{
	g_google_drive_fsm_session->LoadRemoteFile();
}

void upload_sessions()
{
	g_google_drive_fsm_session->startUpload();
}

void download_sessions()
{
	g_google_drive_fsm_session->startDownload();
}

bool getProxyAddr(const string& strAddr, char* strDestAddr, const char* type)
{
	int nStart = strAddr.find(type);
	if (nStart != -1)
	{
		nStart += strlen(type);
		int nLen = strAddr.find(';', nStart + 1) - nStart;
		strcpy(strDestAddr, strAddr.substr(nStart, nLen).c_str());
		return true;
	}
	return false;
}

int getIEProxy(const char* host, int& proxytype, char* strAddr, bool bUserHttps)
{
	USES_CONVERSION;
	bool fAutoProxy = false;
	WINHTTP_PROXY_INFO autoProxyInfo = { 0 };

	WINHTTP_AUTOPROXY_OPTIONS autoProxyOptions = { 0 };
	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ieProxyConfig = { 0 };
	if (WinHttpGetIEProxyConfigForCurrentUser(&ieProxyConfig))
	{
		if (ieProxyConfig.fAutoDetect)
		{
			fAutoProxy = true;
		}

		if (ieProxyConfig.lpszAutoConfigUrl != NULL)
		{
			fAutoProxy = true;
			autoProxyOptions.lpszAutoConfigUrl = ieProxyConfig.lpszAutoConfigUrl;
		}
	}
	else
	{
		// use autoproxy
		fAutoProxy = true;
	}

	if (fAutoProxy)
	{
		if (autoProxyOptions.lpszAutoConfigUrl != NULL)
		{
			autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
		}
		else
		{
			autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
			autoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
		}

		// basic flags you almost always want
		autoProxyOptions.fAutoLogonIfChallenged = TRUE;

		HINTERNET session = ::WinHttpOpen(0, // no agent string
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			WINHTTP_FLAG_ASYNC);

		// here we reset fAutoProxy in case an auto-proxy isn't actually
		// configured for this url

		fAutoProxy = WinHttpGetProxyForUrl(session, A2W(host), &autoProxyOptions, &autoProxyInfo);
		if (session) WinHttpCloseHandle(session);
	}

	if (fAutoProxy)
	{
		// set proxy options for libcurl based on autoProxyInfo
		//autoProxyInfo.lpszProxy
		//curl_easy_setopt(curl,CURLOPT_PROXY,autoProxyInfo.lpszProxy);
		if (autoProxyInfo.lpszProxy)
		{
			strncpy(strAddr, W2A(autoProxyInfo.lpszProxy), 256);
			proxytype = CURLPROXY_HTTP;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		if (ieProxyConfig.lpszProxy != NULL)
		{
			// IE has an explicit proxy. set proxy options for libcurl here
			// based on ieProxyConfig
			//
			// note that sometimes IE gives just a single or double colon
			// for proxy or bypass list, which means "no proxy"
			strncpy(strAddr, W2A(ieProxyConfig.lpszProxy), 256);
			proxytype = CURLPROXY_HTTP;

			///may be like this: "http=127.0.0.1:8888;https=127.0.0.1:8888;ftp=127.0.0.1:8888;socks=127.0.0.1:8888" "127.0.0.1:8888"
			string strProxyAddr(strAddr);
			if (strProxyAddr.find('=') != -1)
			{
				bool bFind = false;
				if (bUserHttps && getProxyAddr(strProxyAddr, strAddr, "https=")) bFind = true;
				if (bFind == false && getProxyAddr(strProxyAddr, strAddr, "http=")) bFind = true;
				if (bFind == false && getProxyAddr(strProxyAddr, strAddr, "socks="))
				{
					proxytype = CURLPROXY_SOCKS5;
				}
			}
		}
		else
		{
			proxytype = -1;
			// there is no auto proxy and no manually configured proxy
		}
	}

	if (autoProxyInfo.lpszProxy != NULL) GlobalFree(autoProxyInfo.lpszProxy);
	if (autoProxyInfo.lpszProxyBypass != NULL) GlobalFree(autoProxyInfo.lpszProxyBypass);
	//if(autoProxyOptions.lpszAutoConfigUrl != NULL) GlobalFree(autoProxyOptions.lpszAutoConfigUrl);
	if (ieProxyConfig.lpszAutoConfigUrl != NULL) GlobalFree(ieProxyConfig.lpszAutoConfigUrl);
	if (ieProxyConfig.lpszProxy != NULL) GlobalFree(ieProxyConfig.lpszProxy);
	if (ieProxyConfig.lpszProxyBypass != NULL) GlobalFree(ieProxyConfig.lpszProxyBypass);

	return proxytype;
}

static void base64urlencodeNoPadding(string& input)
{
	for (int i = 0; i < input.length(); i++)
	{
		if (input[i] == '+'){ input[i] = '-'; }
		else if (input[i] == '/'){ input[i] = '_'; }
	}

	for (int i = input.length() - 1; i >= 0; i--)
	{
		if (input[i] == '='){ input.erase(i); }
	}
}

static string sha256(std::string& input)
{
	unsigned char buffer[32] = { 0 };
	memset(buffer, 0, sizeof(buffer));
	extern const struct ssh_hash ssh_sha256;
	void* handle = ssh_sha256.init();
	ssh_sha256.bytes(handle, input.c_str(), input.length());
	ssh_sha256.final(handle, buffer);
	std::string out;
	base::Base64Encode(base::StringPiece((char*)buffer, 32), &out);
	base64urlencodeNoPadding(out);
	return out;
}

static std::string randomDataBase64url()
{
	std::string ret = base::Generate256BitRandomBase64String();
	base64urlencodeNoPadding(ret);
	return ret;
}

GoogleDriveFsmSession::GoogleDriveFsmSession()
	: Fsm::Session(getZmodemFsm(), 0)
	, mTcpServer(this)
	, mProgressDlg(NULL)
{
}

GoogleDriveFsmSession::~GoogleDriveFsmSession()
{
	stopProgressDlg();

}

void GoogleDriveFsmSession::LoadRemoteFile()
{
	if (getCurState().getId() != IDLE_STATE){	
		handleEvent(Fsm::FAILED_EVT);
	}
	handleEvent(Fsm::NEXT_EVT);
	set_progress_bar("initializing...", 1);
}

void GoogleDriveFsmSession::startUpload()
{
	if (getCurState().getId() != IDLE_STATE){
		MessageBoxA(NULL, mIsUpload ? "uploading, please wait..." : "downloading, please wait...", "error", MB_OK | MB_TOPMOST);
		return;
	}
	mIsUpload = true;
	handleEvent(Fsm::NEXT_EVT);
}

void GoogleDriveFsmSession::startDownload()
{
	if (getCurState().getId() != IDLE_STATE){
		MessageBoxA(NULL, mIsUpload ? "uploading, please wait..." : "downloading, please wait...", "error", MB_OK | MB_TOPMOST);
		return;
	}
	mIsUpload = false;
	handleEvent(Fsm::NEXT_EVT);
}

static base::Lock fsmLock_;
static std::auto_ptr<Fsm::FiniteStateMachine> fsm_;
Fsm::FiniteStateMachine* GoogleDriveFsmSession::getZmodemFsm()
{
	if (NULL == fsm_.get())
	{
		base::AutoLock lock(fsmLock_);
		if (NULL == fsm_.get())
		{
			curl_global_init(CURL_GLOBAL_SSL);
			Fsm::FiniteStateMachine* fsm = new Fsm::FiniteStateMachine;
			(*fsm) += FSM_STATE(IDLE_STATE);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::initAll);
			(*fsm) += FSM_EVENT(Fsm::EXIT_EVT, &GoogleDriveFsmSession::initAll);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(GET_AUTH_CODE_STATE));

			(*fsm) += FSM_STATE(GET_AUTH_CODE_STATE);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::getAuthCode);
			(*fsm) += FSM_EVENT(NETWORK_INPUT_EVT, &GoogleDriveFsmSession::handleAuthCodeInput);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(GET_ACCESS_TOKEN_STATE));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(GET_ACCESS_TOKEN_STATE);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::getAccessToken);
			(*fsm) += FSM_EVENT(HTTP_SUCCESS_EVT, &GoogleDriveFsmSession::parseAccessToken);
			(*fsm) += FSM_EVENT(HTTP_FAILED_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(GET_SESSION_FOLDER));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(GET_SESSION_FOLDER);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::getSessionFolder);
			(*fsm) += FSM_EVENT(HTTP_SUCCESS_EVT, &GoogleDriveFsmSession::parseSessionFolderInfo);
			(*fsm) += FSM_EVENT(HTTP_FAILED_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(GET_EXIST_SESSIONS_ID));
			(*fsm) += FSM_EVENT(CREATE_SESSION_FOLDER_EVT, CHANGE_STATE(CREATE_SESSION_FOLDER));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) += FSM_EVENT(DONE_EVT, CHANGE_STATE(DOWNLOAD_DONE));

			(*fsm) += FSM_STATE(CREATE_SESSION_FOLDER);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::createSessionFolder);
			(*fsm) += FSM_EVENT(HTTP_SUCCESS_EVT, &GoogleDriveFsmSession::parseCreateSessionFolderInfo);
			(*fsm) += FSM_EVENT(HTTP_FAILED_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) += FSM_EVENT(NEXT_EVT, CHANGE_STATE(CHECK_ACTION));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(GET_EXIST_SESSIONS_ID);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::getExistSessionsId);
			(*fsm) += FSM_EVENT(HTTP_SUCCESS_EVT, &GoogleDriveFsmSession::parseSessionsId);
			(*fsm) += FSM_EVENT(HTTP_FAILED_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) += FSM_EVENT(NEXT_EVT, CHANGE_STATE(CHECK_ACTION));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(CHECK_ACTION);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::checkAction);
			(*fsm) += FSM_EVENT(UPLOAD_EVT, CHANGE_STATE(UPLOAD_SESSION));
			(*fsm) += FSM_EVENT(DOWNLOAD_EVT, CHANGE_STATE(DOWNLOAD_SESSION));
			(*fsm) += FSM_EVENT(DELETE_EVT, CHANGE_STATE(DELETE_SESSIONS));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(UPLOAD_SESSION);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::uploadSession);
			(*fsm) += FSM_EVENT(HTTP_SUCCESS_EVT, &GoogleDriveFsmSession::parseUploadSession);
			(*fsm) += FSM_EVENT(HTTP_FAILED_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) += FSM_EVENT(DONE_EVT, CHANGE_STATE(CHECK_ACTION));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(DOWNLOAD_SESSION);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::downloadSession);
			(*fsm) += FSM_EVENT(HTTP_SUCCESS_EVT, &GoogleDriveFsmSession::parseDownloadSession);
			(*fsm) += FSM_EVENT(HTTP_FAILED_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(DOWNLOAD_SESSION));
			(*fsm) += FSM_EVENT(DONE_EVT, CHANGE_STATE(CHECK_ACTION));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			fsm_.reset(fsm);
		}

	}
	return fsm_.get();
}

void GoogleDriveFsmSession::initAll()
{
	mRsp.clear();
	mState.clear();
	mCodeVerifier.clear();
	mRedirectUrl.clear();
	mAuthCodeInput.clear();
	mAuthCode.clear();
	mTcpServer.stop();
	mAccessTokenHeader.clear();

	mHttpUrl.clear();
	mPostData.clear();
	mHttpHeaders.clear();
	mHttpRsp.clear();
	mHttpProxy.clear();
	mHttpProxyType = 0;

	char strAddr[256] = { 0 };
	bool bUserHttps = true;
	getIEProxy("https://www.googleapis.com", mHttpProxyType, strAddr, bUserHttps);
	mHttpProxy = strAddr;

	mSessionFolderId.clear();
	mExistSessionsId.clear();

	mDownloadList.clear();
	mDeleteList.clear();
	mUploadList.clear();
}

void GoogleDriveFsmSession::startProgressDlg()
{
	//CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (void **)&mProgressDlg);
	//mProgressDlg->StartProgressDialog(NULL, NULL, PROGDLG_NORMAL, NULL);
	//mProgressDlg->SetCancelMsg(L"Please wait while the current operation is cleaned up", NULL);
}

void GoogleDriveFsmSession::stopProgressDlg()
{
	//if (mProgressDlg == NULL){ return; }
	//mProgressDlg->StopProgressDialog();
	//mProgressDlg->Release();
	//mProgressDlg = NULL;
}

void GoogleDriveFsmSession::updateProgressDlg(const string& title, const string& desc, int completed, int total)
{
	//USES_CONVERSION;
	//mProgressDlg->SetTitle(A2W(title.c_str()));
	////ppd->SetAnimation(hInstApp, IDA_OPERATION_ANIMATION);
	//char info[1024] = { 0 };
	//snprintf(info, sizeof(info), "(%d/%d)%s", completed+1, total, desc.c_str());
	//mProgressDlg->SetLine(1, A2W(info), false, NULL);
	//mProgressDlg->SetProgress(completed, total);
}

void GoogleDriveFsmSession::getAuthCode()
{
	mState = randomDataBase64url();
	mCodeVerifier = randomDataBase64url();
	mCodeChallenge = sha256(mCodeVerifier);

	int proxytype = 0;
	char strAddr[256] = { 0 };
	bool bUserHttps = true;
	int get_proxy_return = getIEProxy("https://accounts.google.com", proxytype, strAddr, bUserHttps);

	//char notification[2048] = { 0 };
	//snprintf(notification, sizeof(notification),
	//	"1. cost free?\n"
	//	"Yes.(Well, the truth is we are at Google's mercy:))\n"
	//	"\n"
	//	"2. any filter?\n"
	//	"folder/session started with 'tmp' won't be uploaded. It is not configurable.\n"
	//	"\n"
	//	"3. proxy if any?\n"
	//	"IE proxy: %s\n"
	//	"\n"
	//	"4. where to find the uploaded file?\n"
	//	"folder named putty-nd_sessions on your google driver.\n"
	//	"\n"
	//	"5. how to handle conflictions?\n"
	//	"just to replace the session with the same name and leave others alone.\n"
	//	"\n"
	//	"6. how to delete/manager sessions on the cloud?\n"
	//	"exploring to https://drive.google.com/drive/\n"
	//	"\n"
	//	"Click OK to forward, others to abort."
	//	, strAddr[0] == 0 ? "NONE" : strAddr
	//	);
	//if (IDOK != MessageBoxA(NULL, notification, "U MAY WANT TO ASK", MB_OKCANCEL | MB_ICONQUESTION | MB_TOPMOST))
	//{
	//	handleEvent(Fsm::FAILED_EVT);
	//	return;
	//}

	mTcpServer.start();
	int port = mTcpServer.getBindedPort();
	if (port < 0)
	{
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	set_progress_bar("getting the auth code...", 5);

	CURL *curl;
	CURLcode res;
	string response_str;
	char redirectBuff[128] = { 0 };
	snprintf(redirectBuff, sizeof(redirectBuff), "http://127.0.0.1:%d/", port);
	mRedirectUrl = string(redirectBuff);
	char requestUrl[4096] = { 0 };
	snprintf(requestUrl, sizeof(requestUrl), "https://accounts.google.com/o/oauth2/v2/auth?"
		"response_type=code"
		"&redirect_uri=%s"
		"&client_id=%s"
		"&state=%s"
		"&code_challenge=%s"
		"&code_challenge_method=S256"
		"&scope=https://www.googleapis.com/auth/drive.file"
		,
		redirectBuff, client_id, mState.c_str(), mCodeChallenge.c_str());

	ShellExecuteA(NULL, "open", requestUrl, 0, 0, SW_SHOWDEFAULT);
}

void GoogleDriveFsmSession::handleAuthCodeInput()
{
	mAuthCode.clear();
	string state;
	vector<string> strVec;
	base::SplitString(mAuthCodeInput, '&', &strVec);
	vector<string> attrVec;
	for (int i = 0; i < strVec.size(); i++)
	{
		attrVec.clear();
		base::SplitString(strVec[i], '=', &attrVec);
		if (attrVec.size() != 2) continue;
		if (strcmp(attrVec[0].c_str(), "error") == 0)
		{
			handleEvent(Fsm::FAILED_EVT);
			MessageBoxA(NULL, attrVec[1].c_str(), "auth error", MB_OK | MB_TOPMOST);
			return;
		}
		if (strcmp(attrVec[0].c_str(), "code") == 0)
		{
			mAuthCode = attrVec[1];
		}
		else if (strcmp(attrVec[0].c_str(), "state") == 0)
		{
			state = attrVec[1];
		}
	}
	if (mAuthCode.empty() || state.empty())
	{
		handleEvent(Fsm::FAILED_EVT);
		MessageBoxA(NULL, "failed to get auth code or state", "auth error", MB_OK | MB_TOPMOST);
		return;
	}
	if (mState != state)
	{
		handleEvent(Fsm::FAILED_EVT);
		MessageBoxA(NULL, "invalid auth state", "auth error", MB_OK | MB_TOPMOST);
		return;
	}
	handleEvent(Fsm::NEXT_EVT);
}

void GoogleDriveFsmSession::getAccessToken()
{
	set_progress_bar("got the auth code, now getting the access token...", 30);
	{	
		resetHttpData();
		AutoLock lock(mHttpLock);
		char postData[4096] = { 0 };
		snprintf(postData, sizeof(postData),
			"code=%s"
			"&redirect_uri=%s"
			"&client_id=%s"
			"&code_verifier=%s"
			"&client_secret=%s"
			"&grant_type=authorization_code"
			,
			mAuthCode.c_str(), mRedirectUrl.c_str(), client_id, mCodeVerifier.c_str(), client_secret);
		mHttpUrl = "https://www.googleapis.com/oauth2/v4/token";
		mPostData = postData;
		mHttpHeaders.push_back("Content-type: application/x-www-form-urlencoded");
		//mHttpHeaders.push_back("Accept: Accept=text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	}
	g_bg_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::bgHttpRequest, this, "POST"));	
}

void GoogleDriveFsmSession::parseAccessToken()
{
	Document rspJson;
	bool hasError = false;
	{
		AutoLock lock(mHttpLock);
		hasError = rspJson.Parse(mHttpRsp.c_str()).HasParseError();
	}
	if (hasError)
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "parse access token error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	if (!rspJson.HasMember("access_token"))
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "access token not found error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT); 
		return;
	}
	Value& accessTokenValue = rspJson["access_token"];
	if (!accessTokenValue.IsString())
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "access token type error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT); 
		return;
	}
	string accessToken;
	accessToken = accessTokenValue.GetString();

	mAccessTokenHeader = "Authorization: Bearer " + accessToken;
	handleEvent(accessToken.empty() ? Fsm::FAILED_EVT : Fsm::NEXT_EVT);
}

void GoogleDriveFsmSession::getSessionFolder()
{
	set_progress_bar("got the access token, now check if session folder exists...", 60);
	{
		resetHttpData();
		AutoLock lock(mHttpLock);
		mHttpUrl = "https://www.googleapis.com/drive/v2/files?q=title+%3d+%27putty-nd_sessions%27+and+trashed+%3d+false+and+%27root%27+in+parents&orderBy=createdDate";
		//mHttpUrl = "https://www.googleapis.com/drive/v2/files?q=title+%3d+%27aa%2ftest%27+and+trashed+%3d+false+and+%27root%27+in+parents&orderBy=createdDate";
		mHttpHeaders.push_back(mAccessTokenHeader);
		mHttpHeaders.push_back("Accept: Accept=text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	}
	g_bg_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::bgHttpRequest, this, "GET"));
}

void GoogleDriveFsmSession::parseSessionFolderInfo()
{
	Document rspJson;
	bool hasError = false;
	{
		AutoLock lock(mHttpLock);
		hasError = rspJson.Parse(mHttpRsp.c_str()).HasParseError();
	}
	if (hasError)
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "parse response json error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	if (!rspJson.HasMember("items"))
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "json missing value", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	Value& itemsValue = rspJson["items"];
	if (!itemsValue.IsArray())
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "value type error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	int size = itemsValue.Size();
	if (size == 0)
	{
		handleEvent(CREATE_SESSION_FOLDER_EVT);
		return;
	}

	Value& folderValue = itemsValue[0];
	if (!folderValue.HasMember("id"))
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "folder id not found", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}

	Value& foldIdValue = folderValue["id"];
	if (!foldIdValue.IsString())
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "folder id type error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	mSessionFolderId = foldIdValue.GetString();
	handleEvent(Fsm::NEXT_EVT);

}

void GoogleDriveFsmSession::createSessionFolder()
{
	set_progress_bar("init session folder...", 80);
	{
		resetHttpData();
		AutoLock lock(mHttpLock);
		mHttpUrl = "https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart";
		mPostData = "--foo_bar_baz\n"
			"Content-Type: application/json; charset=UTF-8 \n"
			"\n"
			"{ 'name': 'putty-nd_sessions', 'mimeType': 'application/vnd.google-apps.folder'} \n"
			"\n"
			"--foo_bar_baz\n"
			"Content-Type: application/vnd.google-apps.folder\n"
			"\n"
			"--foo_bar_baz--\n";
		mHttpHeaders.push_back(mAccessTokenHeader);
		mHttpHeaders.push_back("Content-Type: multipart/related; boundary=foo_bar_baz");
		mHttpHeaders.push_back("Cache-Control: no-cache");
		mHttpHeaders.push_back("Accept: Accept=text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	}
	g_bg_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::bgHttpRequest, this, "POST"));
}

void GoogleDriveFsmSession::parseCreateSessionFolderInfo()
{
	Document rspJson;
	bool hasError = false;
	{
		AutoLock lock(mHttpLock);
		hasError = rspJson.Parse(mHttpRsp.c_str()).HasParseError();
	}
	if (hasError)
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "parse response json error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	if (!rspJson.HasMember("id"))
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "json missing value", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	
	Value& foldIdValue = rspJson["id"];
	if (!foldIdValue.IsString())
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "folder id type error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	mSessionFolderId = foldIdValue.GetString();

	set_progress_bar("session folder inited", 100);
	handleEvent(NEXT_EVT);
}

void GoogleDriveFsmSession::getExistSessionsId()
{
	set_progress_bar("collecting existing sessions' google file id...", 70);
	{
		resetHttpData();
		AutoLock lock(mHttpLock);
		mHttpUrl = "https://www.googleapis.com/drive/v2/files?q=mimeType+%3d+%27text%2fputtysess%27+and+trashed+%3d+false+and+%27" + mSessionFolderId + "%27+in+parents"
			"&orderBy=createdDate"
			"&maxResults=50";
		mHttpHeaders.push_back(mAccessTokenHeader);
		mHttpHeaders.push_back("Accept: Accept=text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	}
	g_bg_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::bgHttpRequest, this, "GET"));
}

void GoogleDriveFsmSession::parseSessionsId()
{
	Document rspJson;
	bool hasError = false;
	{
		AutoLock lock(mHttpLock);
		hasError = rspJson.Parse(mHttpRsp.c_str()).HasParseError();
	}
	if (hasError)
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "parse response json error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	if (!rspJson.HasMember("items"))
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "json missing value", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	Value& itemsValue = rspJson["items"];
	if (!itemsValue.IsArray())
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "value type error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	int size = itemsValue.Size();
	for (int i = 0; i < size; i++)
	{
		Value& sessionInfoValue = itemsValue[i];
		if (!sessionInfoValue.HasMember("id") || !sessionInfoValue.HasMember("title"))
		{
			MessageBoxA(NULL, mHttpRsp.c_str(), "session id/title not found", MB_OK | MB_TOPMOST);
			handleEvent(Fsm::FAILED_EVT);
			return;
		}
		
		Value& sessionIdValue = sessionInfoValue["id"];
		Value& sessionTitleValue = sessionInfoValue["title"];
		if (!sessionIdValue.IsString() || !sessionTitleValue.IsString())
		{
			MessageBoxA(NULL, mHttpRsp.c_str(), "session id/title type error", MB_OK | MB_TOPMOST);
			handleEvent(Fsm::FAILED_EVT);
			return;
		}
		mExistSessionsId[sessionTitleValue.GetString()] = sessionIdValue.GetString();
	}

	if (!rspJson.HasMember("nextLink"))
	{
		char msg[128] = { 0 };
		snprintf(msg, sizeof(msg), "collected %d sessions' google file id.", mExistSessionsId.size());
		set_progress_bar(msg, 100);
		handleEvent(NEXT_EVT);
		return;
	}

	char msg[128] = { 0 };
	snprintf(msg, sizeof(msg), "got %d files, collecting rest sessions' google file id...", mExistSessionsId.size());
	int progress = mExistSessionsId.size() / 10 + 70;
	set_progress_bar(msg, progress > 99 ? 99 : progress);
	Value& nextLinkValue = rspJson["nextLink"];
	if (!nextLinkValue.IsString())
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "nextLink type error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	string nextLink = nextLinkValue.GetString();
	{
		resetHttpData();
		AutoLock lock(mHttpLock);
		mHttpUrl = nextLink;
		mHttpHeaders.push_back(mAccessTokenHeader);
		mHttpHeaders.push_back("Accept: Accept=text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	}
	g_bg_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::bgHttpRequest, this, "GET"));
}

void GoogleDriveFsmSession::checkAction()
{
	if (!mDownloadList.empty())
	{
		handleEvent(DOWNLOAD_EVT);
		return;
	}
	if (!mDeleteList.empty())
	{
		handleEvent(DELETE_EVT);
		return;
	}
	if (!mUploadList.empty())
	{
		handleEvent(UPLOAD_EVT);
		return;
	}
}

void GoogleDriveFsmSession::uploadSession()
{
	if (mUploadList.empty())
	{
		handleEvent(DONE_EVT);
		return;
	}

	std::string& sessionName = mUploadList.front();
	MemStore memStore;
	Conf* cfg = conf_new();
	void *sesskey = memStore.open_settings_w(sessionName.c_str(), NULL);
	load_settings(sessionName.c_str(), cfg);
	save_open_settings(&memStore, sesskey, cfg);
	conf_free(cfg);
	stringstream *fp = (stringstream *)sesskey;
	string content = fp->str();
	memStore.close_settings_w(sesskey);

	map<string, string>::iterator it = mExistSessionsId.find(sessionName);
	bool isUpdate = it != mExistSessionsId.end();

	if (mProgressDlg->HasUserCancelled()){ handleEvent(Fsm::FAILED_EVT); return; }
	updateProgressDlg("(2/2)Uploading", sessionName, 0, mUploadList.size());
	{
		resetHttpData();
		AutoLock lock(mHttpLock);
		mHttpUrl = (isUpdate ? (string("https://www.googleapis.com/upload/drive/v2/files/") + it->second) : string("https://www.googleapis.com/upload/drive/v3/files"))
			+"?uploadType=multipart";
		mPostData = "--foo_bar_baz\n"
			"Content-Type: application/json; charset=UTF-8 \n"
			"\n"
			"{ 'name': '" + sessionName + "','parents': ['" + mSessionFolderId + "']}\n"
			"\n"
			"--foo_bar_baz\n"
			"Content-Type: text/puttysess\n"
			"\n"
			+ content + "\n"
			"--foo_bar_baz--\n";
		mHttpHeaders.push_back(mAccessTokenHeader);
		mHttpHeaders.push_back("Content-Type: multipart/related; boundary=foo_bar_baz");
		mHttpHeaders.push_back("Cache-Control: no-cache");
		mHttpHeaders.push_back("Accept: Accept=text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	}
	const char* method = (isUpdate ? "PUT" : "POST");
	g_bg_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::bgHttpRequest, this, method));
}

void GoogleDriveFsmSession::parseUploadSession()
{
	Document rspJson;
	bool hasError = false;
	{
		AutoLock lock(mHttpLock);
		hasError = rspJson.Parse(mHttpRsp.c_str()).HasParseError();
	}
	if (hasError)
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "parse response json error", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	if (!rspJson.HasMember("id"))
	{
		MessageBoxA(NULL, mHttpRsp.c_str(), "upload session failed", MB_OK | MB_TOPMOST);
		handleEvent(Fsm::FAILED_EVT);
		return;
	}
	
	mUploadList.pop_front();
	handleEvent(Fsm::ENTRY_EVT);
}

void GoogleDriveFsmSession::downloadSession()
{
	if (mDownloadList.empty())
	{ 
		handleEvent(DONE_EVT); 
		return; 
	}
	std::string& sessionName = mDownloadList.front();
	map<string, string>::iterator it = mExistSessionsId.find(sessionName);
	if (it == mExistSessionsId.end()){	
		handleEvent(HTTP_FAILED_EVT);
		return; 
	}

	if (mProgressDlg->HasUserCancelled()){ handleEvent(Fsm::FAILED_EVT); return; }
	updateProgressDlg("(2/2)Downloading", it->first.c_str(), 0, mExistSessionsId.size());
	{
		resetHttpData();
		AutoLock lock(mHttpLock);
		mHttpUrl = "https://www.googleapis.com/drive/v2/files/" + it->second + "?alt=media";
		mHttpHeaders.push_back(mAccessTokenHeader);
	}
	g_bg_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::bgHttpRequest, this, "GET"));
}

void GoogleDriveFsmSession::parseDownloadSession()
{
	MemStore store;
	Conf* tmpCfg = conf_new();
	store.input(mHttpRsp.c_str());
	std::string& sessionName = mDownloadList.front();
	void *sesskey = store.open_settings_r(sessionName.c_str());
	load_open_settings(&store, sesskey, tmpCfg);
	store.close_settings_r(sesskey);
	save_settings(sessionName.c_str(), tmpCfg);
	conf_free(tmpCfg);

	mDownloadList.pop_front();
	handleEvent(Fsm::ENTRY_EVT);
}

void GoogleDriveFsmSession::downloadDone()
{
	extern HWND hConfigWnd;
	if (hConfigWnd != NULL)
	{
		extern void on_sessions_changed();
		on_sessions_changed();
	}
	stopProgressDlg();
	char info[1024] = { 0 };
	snprintf(info, sizeof(info), "%d sessions are downloaded successfully!", mExistSessionsId.size());
	MessageBoxA(NULL, info, "DownloadDone", MB_OK | MB_TOPMOST);
	handleEvent(Fsm::NEXT_EVT);
}

//-----------------------------------------------------------------------------

void GoogleDriveFsmSession::handleInput(SocketConnectionPtr connection)
{
	if (getCurState().getId() != GET_AUTH_CODE_STATE){ 
		connection->close(); 
		return;
	}

	char buffer[1024];
	unsigned len = 1;
	bool canWrite = true;
	connection->resetHeartbeatTimeoutCounter();
	while (len > 0)
	{
		len = connection->getInput(buffer, sizeof(buffer));
		mRsp.append(buffer, len);
	}
	const char* header = strstr(mRsp.c_str(), " HTTP/1.1");
	if (header != NULL)
	{
		const char* back_msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><head><meta http-equiv='refresh' content='10;url=https://drive.google.com/drive/'></head><body>Please return to the app.</body></html>";
		connection->sendn(back_msg, strlen(back_msg));

		int ignore_pre_len = 6;
		mAuthCodeInput = mRsp.substr(ignore_pre_len, header - mRsp.c_str() - ignore_pre_len);
		mRsp.clear();
		handleEvent(NETWORK_INPUT_EVT);
	}
}

void GoogleDriveFsmSession::handleClose(SocketConnectionPtr theConnection)
{

}

size_t GoogleDriveFsmSession::query_auth_write_cb(void *_ptr, size_t _size, size_t _nmemb, void *_data)
{
	size_t realsize = _size * _nmemb;
	GoogleDriveFsmSession* gdfs = (GoogleDriveFsmSession*)_data;
	AutoLock lock(gdfs->mHttpLock);
	gdfs->mHttpRsp.append((char*)_ptr, realsize);
	return realsize;
}

void GoogleDriveFsmSession::bgHttpRequest(const char* const method)
{
	CURL* curl = curl_easy_init();
	struct curl_slist* headers = NULL;
	{
		AutoLock lock(mHttpLock);
		mHttpRsp.clear();
		mHttpResult = CURLE_OK;
		curl_easy_setopt(curl, CURLOPT_URL, mHttpUrl.c_str());
		if (!mPostData.empty()){ curl_easy_setopt(curl, CURLOPT_POSTFIELDS, mPostData.c_str()); }
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method); //GET/POST/PUT //curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		for (int i = 0; i < mHttpHeaders.size(); i++)
		{
			headers = curl_slist_append(headers, mHttpHeaders[i].c_str());
		}
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, query_auth_write_cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)this);

		if (!mHttpProxy.empty())
		{
			curl_easy_setopt(curl, CURLOPT_PROXY, mHttpProxy.c_str());
			curl_easy_setopt(curl, CURLOPT_PROXYTYPE, mHttpProxyType);
		}
	}

	int count = 0;
	CURLcode res;
	while ((res = curl_easy_perform(curl)) != CURLE_OK && count < 5)
	{ 
		{
			AutoLock lock(mHttpLock);
			mHttpRsp.clear();
		}
		Sleep(1000);
		count++;
	}
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	{
		AutoLock lock(mHttpLock);
		mHttpResult = res;
	}
	g_ui_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::handleHttpRsp, this));
}

void GoogleDriveFsmSession::handleHttpRsp()
{
	if(mHttpResult != CURLE_OK || mHttpRsp.empty())
	{
		handleEvent(HTTP_FAILED_EVT);
	}
	else
	{
		handleEvent(HTTP_SUCCESS_EVT);
	}
}

void GoogleDriveFsmSession::resetHttpData()
{
	AutoLock lock(mHttpLock);
	mHttpUrl.clear();
	mPostData.clear();
	mHttpHeaders.clear();
	mHttpRsp.clear();
	mHttpResult = CURLE_OK;
}



