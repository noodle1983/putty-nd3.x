#include "google_drive_fsm_session.h"
#include "putty.h"
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

void upload_sessions()
{
	g_google_drive_fsm_session->startUpload();
}

static size_t query_auth_write_cb(void *_ptr, size_t _size, size_t _nmemb, void *_data)
{
	size_t realsize = _size * _nmemb;
	string* response_str = (string*)_data;
	response_str->append((char*)_ptr, realsize);
	return realsize;
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
	mHandlingIndex = 0;
}

GoogleDriveFsmSession::~GoogleDriveFsmSession()
{
	stopProgressDlg();

}

void GoogleDriveFsmSession::startUpload()
{
	if (getCurState().getId() != IDLE_STATE){
		MessageBoxA(NULL, mIsUpload ? "uploading, please wait..." : "downloading, please wait...", "error", MB_OK);
		return;
	}
	mIsUpload = true;
	handleEvent(Fsm::NEXT_EVT);
}

void GoogleDriveFsmSession::startDownload()
{
	if (getCurState().getId() != IDLE_STATE){
		MessageBoxA(NULL, mIsUpload ? "uploading, please wait..." : "downloading, please wait...", "error", MB_OK);
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
			Fsm::FiniteStateMachine* fsm = new Fsm::FiniteStateMachine;
			(*fsm) += FSM_STATE(IDLE_STATE);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::initAll);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::stopProgressDlg);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(GET_AUTH_CODE_STATE));

			(*fsm) += FSM_STATE(GET_AUTH_CODE_STATE);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::getAuthCode);
			(*fsm) += FSM_EVENT(NETWORK_INPUT_EVT, &GoogleDriveFsmSession::handleAuthCodeInput);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(GET_ACCESS_TOKEN_STATE));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(GET_ACCESS_TOKEN_STATE);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::getAccessToken);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(GET_GET_SESSION_FOLDER));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(GET_GET_SESSION_FOLDER);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::getSessionFolder);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(GET_EXIST_SESSIONS_ID));
			(*fsm) += FSM_EVENT(CREATE_SESSION_FOLDER_EVT, CHANGE_STATE(CREATE_SESSION_FOLDER));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(CREATE_SESSION_FOLDER);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::createSessionFolder);
			(*fsm) += FSM_EVENT(PREPARE_UPLOAD_EVT, CHANGE_STATE(PREPARE_UPLOAD));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(GET_EXIST_SESSIONS_ID);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::getExistSessionsId);
			(*fsm) += FSM_EVENT(PREPARE_UPLOAD_EVT, CHANGE_STATE(PREPARE_UPLOAD));
			(*fsm) += FSM_EVENT(PREPARE_DOWNLOAD_EVT, CHANGE_STATE(PREPARE_DOWNLOAD));
			(*fsm) += FSM_EVENT(GET_REST_SESSIONS_ID_EVT, CHANGE_STATE(GET_REST_SESSIONS_ID));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(GET_REST_SESSIONS_ID);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::getRestSessionsId);
			(*fsm) += FSM_EVENT(GET_REST_SESSIONS_ID_EVT, CHANGE_STATE(GET_REST_SESSIONS_ID));
			(*fsm) += FSM_EVENT(PREPARE_UPLOAD_EVT, CHANGE_STATE(PREPARE_UPLOAD));
			(*fsm) += FSM_EVENT(PREPARE_DOWNLOAD_EVT, CHANGE_STATE(PREPARE_DOWNLOAD));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(PREPARE_UPLOAD);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::prepareUpload);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(UPLOAD_SESSION));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(UPLOAD_SESSION);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::uploadSession);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(UPLOAD_SESSION));
			(*fsm) += FSM_EVENT(DONE_EVT, CHANGE_STATE(UPLOAD_DONE));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(UPLOAD_DONE);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::uploadDone);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(PREPARE_DOWNLOAD);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::getExistSessionsId);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(DOWNLOAD_SESSION));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(DOWNLOAD_SESSION);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::downloadSession);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(DOWNLOAD_SESSION));
			(*fsm) += FSM_EVENT(DONE_EVT, CHANGE_STATE(DOWNLOAD_DONE));
			(*fsm) += FSM_EVENT(Fsm::FAILED_EVT, CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(DOWNLOAD_DONE);
			(*fsm) += FSM_EVENT(Fsm::ENTRY_EVT, &GoogleDriveFsmSession::downloadDone);
			(*fsm) += FSM_EVENT(Fsm::NEXT_EVT, CHANGE_STATE(IDLE_STATE));
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
}

void GoogleDriveFsmSession::startProgressDlg()
{
	CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (void **)&mProgressDlg);
	mProgressDlg->StartProgressDialog(NULL, NULL, PROGDLG_NORMAL, NULL);
	mProgressDlg->SetCancelMsg(L"Please wait while the current operation is cleaned up", NULL);
}

void GoogleDriveFsmSession::stopProgressDlg()
{
	if (mProgressDlg == NULL){ return; }
	mProgressDlg->StopProgressDialog();
	mProgressDlg->Release();
	mProgressDlg = NULL;
}

void GoogleDriveFsmSession::updateProgressDlg(const string& title, const string& desc, int completed, int total)
{
	USES_CONVERSION;
	mProgressDlg->SetTitle(A2W(title.c_str()));
	//ppd->SetAnimation(hInstApp, IDA_OPERATION_ANIMATION);
	mProgressDlg->SetLine(1, A2W(desc.c_str()), false, NULL);
	mProgressDlg->SetProgress(completed, total);
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

	char notification[2048] = { 0 };
	snprintf(notification, sizeof(notification),
		"1. proxy if any?\n"
		"IE proxy: %s\n"
		"\n"
		"2. how to auth?\n"
		"by google auth2 through default browser.\n"
		"\n"
		"3. where to find the uploaded file?\n"
		"folder named putty-nd_sessions on your google driver.\n"
		"\n"
		"4. how to handle conflictions?\n"
		"just to replace the session with the same name and leave others alone.\n"
		"\n"
		"5. how to delete/manager sessions on the cloud?\n"
		"exploring to https://drive.google.com/drive/my-drive\n"
		"\n"
		"Click OK to forward, others to abort."
		, strAddr[0] == 0 ? "NONE" : strAddr
		);
	if (IDOK != MessageBoxA(NULL, notification, "U MAY WANT TO ASK", MB_OKCANCEL | MB_ICONQUESTION))
	{
		handleEvent(Fsm::FAILED_EVT);
		return;
	}

	startProgressDlg();
	updateProgressDlg("Auth with Google", "getting auth code...", 0, 4);

	mTcpServer.start();
	int port = mTcpServer.getBindedPort();
	if (port < 0)
	{
		handleEvent(Fsm::FAILED_EVT);
		return;
	}

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
			MessageBoxA(NULL, attrVec[1].c_str(), "auth error", MB_OK);
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
		MessageBoxA(NULL, "failed to get auth code or state", "auth error", MB_OK);
		return;
	}
	if (mState != state)
	{
		handleEvent(Fsm::FAILED_EVT);
		MessageBoxA(NULL, "invalid auth state", "auth error", MB_OK);
		return;
	}
	handleEvent(Fsm::NEXT_EVT);
}

void GoogleDriveFsmSession::getAccessToken()
{
	updateProgressDlg("Auth with Google", "getting access token...", 1, 4);
	g_bg_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::bgGetAccessToken, this, mAuthCode, mCodeVerifier, mRedirectUrl));	
}

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
		const char* back_msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><head><meta http-equiv='refresh' content='10;url=https://google.com'></head><body>Please return to the app.</body></html>";
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

//-----------------------------------------------------------------------------

void GoogleDriveFsmSession::bgGetAccessToken(string authCode, string codeVerifier, string redirectUrl)
{
	char postData[4096] = { 0 };
	snprintf(postData, sizeof(postData),
		"code=%s"
		"&redirect_uri=%s"
		"&client_id=%s"
		"&code_verifier=%s"
		"&client_secret=%s"
		"&grant_type=authorization_code"
		,
		authCode.c_str(), redirectUrl.c_str(), client_id, codeVerifier.c_str(), client_secret);

	string response_str;
	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, "https://www.googleapis.com/oauth2/v4/token");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "Content-type: application/x-www-form-urlencoded");
	headers = curl_slist_append(headers, "Accept: Accept=text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, query_auth_write_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_str);
	int proxytype = 0;
	char strAddr[256] = { 0 };
	bool bUserHttps = true;
	int get_proxy_return = getIEProxy("https://www.googleapis.com", proxytype, strAddr, bUserHttps);
	if (get_proxy_return >= 0)
	{
		curl_easy_setopt(curl, CURLOPT_PROXY, strAddr);
		curl_easy_setopt(curl, CURLOPT_PROXYTYPE, proxytype);
	}

	int count = 0;
	CURLcode res;
	while ((res = curl_easy_perform(curl)) != CURLE_OK && count < 5)
	{
		response_str.clear();
		Sleep(1000);
		count++;
	}
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	//parse
	string access_token;
	Document d;
	if (d.Parse(response_str.c_str()).HasParseError())
	{
		MessageBoxA(NULL, response_str.c_str(), "parse access token error", MB_OK);
		g_ui_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::handleAccessToken, this, access_token));
		return;
	}
	if (!d.HasMember("access_token"))
	{
		MessageBoxA(NULL, response_str.c_str(), "access token not found error", MB_OK);
		g_ui_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::handleAccessToken, this, access_token));
		return;
	}
	Value& s = d["access_token"];
	if (!s.IsString())
	{
		MessageBoxA(NULL, response_str.c_str(), "access token type error", MB_OK);
		g_ui_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::handleAccessToken, this, access_token));
		return;
	}
	access_token = s.GetString();
	g_ui_processor->process(0, NEW_PROCESSOR_JOB(&GoogleDriveFsmSession::handleAccessToken, this, access_token));
	

}

void GoogleDriveFsmSession::handleAccessToken(string accessToken)
{
	updateProgressDlg("Auth with Google", "getting access token...", 1, 4);
	mAccessTokenHeader = "Authorization: Bearer " + accessToken;
	handleEvent(accessToken.empty() ? Fsm::FAILED_EVT : Fsm::NEXT_EVT);
}


