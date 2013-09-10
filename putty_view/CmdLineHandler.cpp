#include "CmdLineHandler.h"
#include "window_interface.h"
#include <atlconv.h>
#include "browser.h"
#include "browser_list.h"
#include "browser_window.h"
#include "base/timer.h"
#include "string.h"
#include "Security.h"
#include "native_putty_common.h"
#include "misc.h"
void fatalbox(char *fmt, ...);
#include "putty.h"
#include "storage.h"
extern Config cfg;
extern IStore* gStorage;

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
	memset(cmdLine_, 0, sizeof(cmdLine_));
	isLeaderStartWithCmd_ = false;
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
		//parse cmd line if there is any
		if (strlen (cmdLine_) > 0){
			process_cmdline(cmdLine_);
			isLeaderStartWithCmd_ = true;
		}
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
		if (sharedBuffer_[1] == 0){
			WindowInterface::GetInstance()->createNewSession();
		}else{
			process_cmdline(sharedBuffer_ + 1);
			WindowInterface::GetInstance()->createNewSessionWithGlobalCfg();
		}
		memset(sharedBuffer_, 0, SHARED_MEM_SIZE);
		ReleaseMutex(sharedMemMutex_);
	}
}


//follower method
void CmdLineHandler::sendMsgToLeader()
{
	{
		HWND hwnd = FindWindowA("PuTTY-ND2_ConfigBox", NULL);
		if (hwnd)
		{
			bringToForeground(hwnd);
			return;
		}
		DWORD wait_result = WaitForSingleObject(sharedMemMutex_, 0);
		if (WAIT_OBJECT_0 != wait_result)
		{
			return;
		}

		memset(sharedBuffer_, 0, SHARED_MEM_SIZE);
		sharedBuffer_[0] = COMMAND_CMD_LINE;
		strncpy(sharedBuffer_+1, cmdLine_, SHARED_MEM_SIZE-2);

		ReleaseMutex(sharedMemMutex_);
	}
}

/*
 * Process the command line.
 */
void CmdLineHandler::process_cmdline(LPSTR cmdline)
{
	USES_CONVERSION;
	char *p;
	int got_host = 0;
	/* By default, we bring up the config dialog, rather than launching
	 * a session. This gets set to TRUE if something happens to change
	 * that (e.g., a hostname is specified on the command-line). */
	int allow_launch = FALSE;

	default_protocol = be_default_protocol;
	/* Find the appropriate default port. */
	{
	    Backend *b = backend_from_proto(default_protocol);
	    default_port = 0; /* illegal */
	    if (b)
		default_port = b->default_port;
	}
	cfg.logtype = LGTYP_NONE;

	do_defaults(NULL, &cfg);

	p = cmdline;

	/*
	 * Process a couple of command-line options which are more
	 * easily dealt with before the line is broken up into words.
	 * These are the old-fashioned but convenient @sessionname and
	 * the internal-use-only &sharedmemoryhandle, neither of which
	 * are combined with anything else.
	 */
	while (*p && isspace(*p))
	    p++;
	if (*p == '@') {
            /*
             * An initial @ means that the whole of the rest of the
             * command line should be treated as the name of a saved
             * session, with _no quoting or escaping_. This makes it a
             * very convenient means of automated saved-session
             * launching, via IDM_SAVEDSESS or Windows 7 jump lists.
             */
	    int i = strlen(p);
	    while (i > 1 && isspace(p[i - 1]))
		i--;
	    p[i] = '\0';
	    do_defaults(p + 1, &cfg);
	    if (!cfg_launchable(&cfg) && !do_config()) {
			return;
	    }
	    allow_launch = TRUE;    /* allow it to be launched directly */
	} else if (*p == '&') {
	    /*
	     * An initial & means we've been given a command line
	     * containing the hex value of a HANDLE for a file
	     * mapping object, which we must then extract as a
	     * config.
	     */
	    HANDLE filemap;
	    Config *cp;
	    if (sscanf(p + 1, "%p", &filemap) == 1 &&
		(cp = (Config*)MapViewOfFile(filemap, FILE_MAP_READ,
				    0, 0, sizeof(Config))) != NULL) {
		cfg = *cp;
		UnmapViewOfFile(cp);
		CloseHandle(filemap);
	    } else if (!do_config()) {
		cleanup_exit(0);
	    }
	    allow_launch = TRUE;
	} else {
	    /*
	     * Otherwise, break up the command line and deal with
	     * it sensibly.
	     */
	    int argc, i;
	    char **argv;
	    
	    split_into_argv(cmdline, &argc, &argv, NULL);

	    for (i = 0; i < argc; i++) {
		char *p = argv[i];
		int ret;

		ret = cmdline_process_param(p, i+1<argc?argv[i+1]:NULL,
					    1, &cfg);
		if (ret == -2) {
		    cmdline_error("option \"%s\" requires an argument", p);
		} else if (ret == 2) {
		    i++;	       /* skip next argument */
		} else if (ret == 1) {
		    continue;	       /* nothing further needs doing */
		} else if (!strcmp(p, "-cleanup") ||
			   !strcmp(p, "-cleanup-during-uninstall")) {
		    /*
		     * `putty -cleanup'. Remove all registry
		     * entries associated with PuTTY, and also find
		     * and delete the random seed file.
		     */
		    char *s1, *s2;
		    /* Are we being invoked from an uninstaller? */
		    if (!strcmp(p, "-cleanup-during-uninstall")) {
			s1 = dupprintf("Remove saved sessions and random seed file?\n"
				       "\n"
				       "If you hit Yes, ALL Registry entries associated\n"
				       "with %s will be removed, as well as the\n"
				       "random seed file. THIS PROCESS WILL\n"
				       "DESTROY YOUR SAVED SESSIONS.\n"
				       "(This only affects the currently logged-in user.)\n"
				       "\n"
				       "If you hit No, uninstallation will proceed, but\n"
				       "saved sessions etc will be left on the machine.",
				       appname);
			s2 = dupprintf("%s Uninstallation", appname);
		    } else {
			s1 = dupprintf("This procedure will remove ALL Registry entries\n"
				       "associated with %s, and will also remove\n"
				       "the random seed file. (This only affects the\n"
				       "currently logged-in user.)\n"
				       "\n"
				       "THIS PROCESS WILL DESTROY YOUR SAVED SESSIONS.\n"
				       "Are you really sure you want to continue?",
				       appname);
			s2 = dupprintf("%s Warning", appname);
		    }
		    //if (message_box(A2W(s1), A2W(s2),
			//	    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2,
			//	    HELPCTXID(option_cleanup)) == IDYES) {
			if (MessageBox(WindowInterface::GetInstance()->getNativeTopWnd(), A2W(s1), A2W(s2),
				    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES) {
			gStorage->cleanup_all();
		    }
		    sfree(s1);
		    sfree(s2);
		    exit(0);
		} else if (!strcmp(p, "-pgpfp")) {
		    pgp_fingerprints();
		    exit(1);
		} else if (*p != '-') {
		    char *q = p;
		    if (got_host) {
			/*
			 * If we already have a host name, treat
			 * this argument as a port number. NB we
			 * have to treat this as a saved -P
			 * argument, so that it will be deferred
			 * until it's a good moment to run it.
			 */
			int ret = cmdline_process_param("-P", p, 1, &cfg);
			assert(ret == 2);
		    } else if (!strncmp(q, "telnet:", 7)) {
			/*
			 * If the hostname starts with "telnet:",
			 * set the protocol to Telnet and process
			 * the string as a Telnet URL.
			 */
			char c;

			q += 7;
			if (q[0] == '/' && q[1] == '/')
			    q += 2;
			cfg.protocol = PROT_TELNET;
			p = q;
			while (*p && *p != ':' && *p != '/')
			    p++;
			c = *p;
			if (*p)
			    *p++ = '\0';
			if (c == ':')
			    cfg.port = atoi(p);
			else
			    cfg.port = -1;
			strncpy(cfg.host, q, sizeof(cfg.host) - 1);
			cfg.host[sizeof(cfg.host) - 1] = '\0';
			got_host = 1;
		    } else {
			/*
			 * Otherwise, treat this argument as a host
			 * name.
			 */
			while (*p && !isspace(*p))
			    p++;
			if (*p)
			    *p++ = '\0';
			strncpy(cfg.host, q, sizeof(cfg.host) - 1);
			cfg.host[sizeof(cfg.host) - 1] = '\0';
			got_host = 1;
		    }
		} else {
		    cmdline_error("unknown option \"%s\"", p);
		}
	    }
	}

	cmdline_run_saved(&cfg);

	if (loaded_session || got_host)
	    allow_launch = TRUE;

	if ((!allow_launch || !cfg_launchable(&cfg)) && !do_config()) {
	    return ;
	}

	adjust_host(&cfg);

   if (!strcmp(cfg.session_name, DEFAULT_SESSION_NAME)){
      if (cfg.protocol == PROT_SERIAL)
         snprintf(cfg.session_name, sizeof(cfg.session_name),  
            "tmp#%s:%d", cfg.serline, cfg.serspeed);
      else
         snprintf(cfg.session_name, sizeof(cfg.session_name),  
            "tmp#%s:%d", cfg.host, cfg.port);
	  save_settings(cfg.session_name, &cfg);
   }
}





