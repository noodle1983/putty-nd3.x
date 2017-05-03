#include "online_session_manager.h"
#include "putty.h"
#include "../adb/AdbManager.h"
#include <Windows.h>
#include "atlconv.h" 
extern "C"{
#include <curl/curl.h>
}

OnlineSessionManager::OnlineSessionManager()
{
	mHandlingIndex = 0;
}

OnlineSessionManager::~OnlineSessionManager()
{

}

void upload_sessions()
{
	g_online_session_manager->upload_sessions();
}
int OnlineSessionManager::upload_sessions()
{
	//if (!mWaitingList.empty()){ return -1; }
	//
	//struct sesslist sesslist;
	//get_sesslist(&sesslist, TRUE);
	//for (int i = 0; i < sesslist.nsessions; i++) {
	//	if (strcmp(sesslist.sessions[i], DEFAULT_SESSION_NAME) == 0){ continue; }
	//	if (strcmp(sesslist.sessions[i], ANDROID_DIR_FOLDER_NAME) == 0){ continue; }
	//	mWaitingList.push_back(sesslist.sessions[i]);
	//}
	//get_sesslist(&sesslist, FALSE);
	//
	//mHandlingIndex = 0;
	//if (!mWaitingList.empty()){ handle_waiting_list(); }
	handle_waiting_list();
	return 0;
}

void OnlineSessionManager::handle_waiting_list()
{
	//if (mWaitingList.empty()) return ;
	//if (mHandlingIndex < 0 || mHandlingIndex >= mWaitingList.size()) return;
	//string& session_name = mWaitingList[mHandlingIndex];
	//
	//char tmp_file_path[512] = { 0 };
	//GetTempPathA(sizeof(tmp_file_path)-1, tmp_file_path);
	//snprintf(tmp_file_path, sizeof(tmp_file_path)-1, "%s\\%s", tmp_file_path, "upload.sess.tmp");
	//backup_settings(session_name.c_str(), tmp_file_path);
	//g_adbm_processor->process(0, NEW_PROCESSOR_JOB(OnlineSessionManager::upload_file, tmp_file_path));

	g_adbm_processor->process(0, NEW_PROCESSOR_JOB(OnlineSessionManager::upload_file, ""));
}

static size_t query_auth_write_cb(void *_ptr, size_t _size, size_t _nmemb, void *_data)
{
	size_t realsize = _size * _nmemb;
	string* response_str = (string*)_data;
	response_str->append((char*)_ptr, realsize);
	return realsize;
}

void OnlineSessionManager::upload_file(string file)
{
	CURL *curl;
	CURLcode res;
	string response_str;

	curl = curl_easy_init();
	assert(curl);
	curl_easy_setopt(curl, CURLOPT_URL, "https://www.taobao.com");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "cmd=time");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, query_auth_write_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_str);

	int count = 0;
	while ((res = curl_easy_perform(curl)) != CURLE_OK && count < 5)
	{
		response_str.clear();
		Sleep(1000);
		count++;
	}
	curl_easy_cleanup(curl);
	g_ui_processor->process(NEW_PROCESSOR_JOB(OnlineSessionManager::upload_file_done, res == CURLE_OK, response_str));
}


void OnlineSessionManager::upload_file_done(bool is_success, string response)
{
	USES_CONVERSION;
	printf("%s", response.c_str());
	MessageBox(NULL, A2W(response.c_str()), L"test", MB_OK);
}

void OnlineSessionManager::download_sessions()
{

}