#include "CmdLineHandler.h"
#include <atlconv.h>
#include "browser.h"
#include "browser_list.h"
#include "browser_window.h"
#include "base/timer.h"
#include "string.h"
#include "Security.h"
#include "native_putty_common.h"
void fatalbox(char *fmt, ...);

const char* const CmdLineHandler::SHARED_MEM_NAME = "PuttySharedMem";
const char* const CmdLineHandler::SHARED_MEM_MUTEX_NAME = "PuttySharedMemMutex";

CmdLineHandler::CmdLineHandler()
{
	USES_CONVERSION;
	char userId[128] = {0};
	ULONG userIdLength = sizeof userId;
	if (0 == GetUserNameExA(NameSamCompatible, userId, &userIdLength)){
		ErrorExit("GetUserId");
		return;
	}
	for (int i = 0; i < userIdLength; i++){
		if ((userId[i] >= '0' && userId[i] <= '9')
			|| (userId[i] >= 'a' && userId[i] <= 'z')
			|| (userId[i] >= 'A' && userId[i] <= 'Z'))
			continue;
		userId[i] = '_';
	}
	_snprintf(userShareMemName_, sizeof(userShareMemName_), "%s_%s", SHARED_MEM_NAME, userId);
	_snprintf(userShareMemMutexName_, sizeof(userShareMemMutexName_), "%s_%s", SHARED_MEM_MUTEX_NAME, userId);
	sharedBuffer_ = NULL;
	sharedMemMutex_ = CreateMutex(NULL,FALSE, A2W(userShareMemMutexName_));
}

CmdLineHandler::~CmdLineHandler()
{
	if(checkMemTimer_.IsRunning()){
        checkMemTimer_.Stop();
	}
	if (sharedBuffer_){
		UnmapViewOfFile(sharedBuffer_);
		CloseHandle(sharedMemHandle_);
		sharedBuffer_ = NULL;
	}
	CloseHandle(sharedMemMutex_);
}

void CmdLineHandler::handleCmd()
{
	if (toBeLeader()){
		//start timer
		checkMemTimer_.Start(
                base::TimeDelta::FromMilliseconds(TIMER_INTERVAL), this,
                &CmdLineHandler::leaderTimerCallback);

	}else{
		sendMsgToLeader();
		exit(0);
	}

}


bool CmdLineHandler::toBeLeader()
{
	USES_CONVERSION;
	bool ret = true;
	 //create share memory  
    sharedMemHandle_ = CreateFileMapping(INVALID_HANDLE_VALUE,  
        NULL,  
        PAGE_READWRITE,  
        0,  
        SHARED_MEM_SIZE,  
        A2W(userShareMemName_));  
    if (NULL == sharedMemHandle_)  
    {  
        fatalbox("%s", "can't get shared memory handle!");  
		exit(-1);
    }
	sharedType_ =  (GetLastError() == ERROR_ALREADY_EXISTS) ? SHARED_TYPE_FOLLOWER : SHARED_TYPE_LEADER;

	sharedBuffer_ = (char* ) MapViewOfFile(sharedMemHandle_,   // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
        0,
        0,
        SHARED_MEM_SIZE);
	if (NULL == sharedBuffer_){
		fatalbox("%s", "can't get shared memory!");  
		CloseHandle(sharedMemHandle_);
		exit(-1);
	}
	
	if (isLeader()){
		memset(sharedBuffer_, 0, SHARED_MEM_SIZE);
	}
	return isLeader();
}


//leader method
void CmdLineHandler::leaderTimerCallback()
{
	if (*sharedBuffer_ == 0){
		//no msg pending
		return;
	}
	{
		WaitForSingleObject(sharedMemMutex_, INFINITE);
		sharedBuffer_[0] = 0;
		ReleaseMutex(sharedMemMutex_);
	}
	createNewSession();
}


//follower method
void CmdLineHandler::sendMsgToLeader()
{
	{
		WaitForSingleObject(sharedMemMutex_, INFINITE);
		sharedBuffer_[0] = 1;
		ReleaseMutex(sharedMemMutex_);
	}
}


void CmdLineHandler::createNewSession()
{
	Browser* browser = BrowserList::GetLastActive();
	if (browser == NULL){
		fatalbox("%s", "last ative window is not found");
		return ;
	}
	browser->AddBlankTab(true);
	browser->window()->Show();
}

void CmdLineHandler::dupCurSession()
{
	Browser* browser = BrowserList::GetLastActive();
	if (browser == NULL){
		fatalbox("%s", "last ative window is not found");
		return ;
	}
	browser->DuplicateCurrentTab();
	browser->window()->Show();
}

