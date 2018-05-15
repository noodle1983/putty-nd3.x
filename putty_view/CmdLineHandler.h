#ifndef CMDLINEHANDLER_H
#define CMDLINEHANDLER_H

#include "base/memory/scoped_ptr.h"
#include "base/timer.h"
#include "base/memory/singleton.h"
#include <WinBase.h>
#include <Windows.h>

#include "Singleton.hpp"
#include <stdarg.h>
class FileLogger
{
public:
	static FileLogger* GetInstance(){
		return Singleton<FileLogger>::get();
	}
	FileLogger()
	{
		file_handle_ = fopen("putty-nd_debug.log", "a");
	};
	virtual ~FileLogger()
	{
		if (file_handle_)
		{
			fclose(file_handle_);
		}
	}

	void log(char* _fmt, ...)
	{
		if (file_handle_ == NULL) return;

		char log_buf[2048] = { 0 };
		va_list arg_list;
		va_start(arg_list, _fmt);
		int i = vsnprintf(log_buf, sizeof(log_buf), _fmt, arg_list);
		va_end(arg_list);

		if (i > 0)
		{
			fwrite(log_buf, sizeof(char), i, file_handle_);
			fflush(file_handle_);
		}

	}

private:
	FILE* file_handle_;
};
#define g_file_logger Singleton<FileLogger>::get()

class CmdLineHandler{
public:
	enum{SHARED_MEM_SIZE = 512};
	enum{TIMER_INTERVAL = 50}; //in ms
	enum{
		COMMAND_NONE = 0,
		COMMAND_CMD_LINE = 1
	};
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
	int process_cmdline(LPSTR cmdline);

	//follower method
	void sendMsgToLeader();

	//other
	void setCmdline(char* cmdline)
	{ strncpy(cmdLine_, cmdline, sizeof(cmdLine_) -1);}

	bool isLeaderStartWithCmd(){
		return isLeaderStartWithCmd_;
	}



private:
	SharedType sharedType_;
	HANDLE sharedMemHandle_;
	HANDLE sharedMemMutex_;
	char* sharedBuffer_;
	base::RepeatingTimer<CmdLineHandler> checkMemTimer_;
	char userShareMemName_[128];
	char userShareMemMutexName_[128];
	char cmdLine_[SHARED_MEM_SIZE-1];
	bool isLeaderStartWithCmd_;

	friend struct DefaultSingletonTraits<CmdLineHandler>;
	DISALLOW_COPY_AND_ASSIGN(CmdLineHandler);

};

#endif /* CMDLINEHANDLER_H */
