#ifndef CMDLINEHANDLER_H
#define CMDLINEHANDLER_H

#include "base/memory/scoped_ptr.h"
#include "base/timer.h"
#include "base/memory/singleton.h"
#include <WinBase.h>
#include <Windows.h>

class CmdLineHandler{
public:
	enum{SHARED_MEM_SIZE = 512};
	enum{TIMER_INTERVAL = 100}; //in ms
	enum SharedType{SHARED_TYPE_LEADER = 0, SHARED_TYPE_FOLLOWER = 1};
	static const char* const SHARED_MEM_NAME;
	static const char* const SHARED_MEM_MUTEX_NAME;
	static CmdLineHandler* GetInstance(){
		return Singleton<CmdLineHandler>::get();
	}

	CmdLineHandler();
	~CmdLineHandler();

	//common method
	void handleCmd();
	bool toBeLeader(); //create or open shared mem handle
	bool isLeader(){return sharedType_ == SHARED_TYPE_LEADER;};

	//leader method
	void leaderTimerCallback();

	//follower method
	void sendMsgToLeader();

private:
	SharedType sharedType_;
	HANDLE sharedMemHandle_;
	HANDLE sharedMemMutex_;
	char* sharedBuffer_;
	base::RepeatingTimer<CmdLineHandler> checkMemTimer_;
	char userShareMemName_[128];
	char userShareMemMutexName_[128];

	friend struct DefaultSingletonTraits<CmdLineHandler>;
	DISALLOW_COPY_AND_ASSIGN(CmdLineHandler);

};

#endif /* CMDLINEHANDLER_H */
