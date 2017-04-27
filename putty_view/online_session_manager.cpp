#include "online_session_manager.h"
#include "putty.h"
#include "../adb/AdbManager.h"
#include <Windows.h>


OnlineSessionManager::OnlineSessionManager()
{
	mHandlingIndex = 0;
}

OnlineSessionManager::~OnlineSessionManager()
{

}

int OnlineSessionManager::upload_sessions()
{
	if (!mWaitingList.empty()){ return -1; }

	struct sesslist sesslist;
	get_sesslist(&sesslist, TRUE);
	for (int i = 0; i < sesslist.nsessions; i++) {
		if (strcmp(sesslist.sessions[i], DEFAULT_SESSION_NAME) == 0){ continue; }
		if (strcmp(sesslist.sessions[i], ANDROID_DIR_FOLDER_NAME) == 0){ continue; }
		mWaitingList.push_back(sesslist.sessions[i]);
	}
	get_sesslist(&sesslist, FALSE);

	mHandlingIndex = 0;
	if (!mWaitingList.empty()){ handle_waiting_list(); }
	return 0;
}

void OnlineSessionManager::handle_waiting_list()
{
	if (mWaitingList.empty()) return ;
	if (mHandlingIndex < 0 || mHandlingIndex >= mWaitingList.size()) return;
	string& session_name = mWaitingList[mHandlingIndex];

	char tmp_file_path[512] = { 0 };
	GetTempPathA(sizeof(tmp_file_path)-1, tmp_file_path);
	snprintf(tmp_file_path, sizeof(tmp_file_path)-1, "%s\\%s", tmp_file_path, "upload.sess.tmp");
	backup_settings(session_name.c_str(), tmp_file_path);
	g_adbm_processor->process(0, NEW_PROCESSOR_JOB(OnlineSessionManager::upload_file, tmp_file_path));
}

void OnlineSessionManager::upload_file(string file)
{

}


void OnlineSessionManager::upload_file_done(bool is_success)
{

}

void OnlineSessionManager::download_sessions()
{

}