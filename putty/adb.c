/*
 * "adb" backend.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "putty.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define ADB_MAX_BACKLOG 4096

#include <windows.h>
#include <stdio.h>

static ULONG PipeSerialNumber = 0;

static BOOL MyCreatePipeEx(
	OUT LPHANDLE lpReadPipe,
	OUT LPHANDLE lpWritePipe,
	IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
	IN DWORD nSize)
{
	HANDLE ReadPipeHandle, WritePipeHandle;
	DWORD dwError;
	char PipeNameBuffer[MAX_PATH];

	//
	// Only one valid OpenMode flag - FILE_FLAG_OVERLAPPED
	//

	DWORD dwReadMode = FILE_FLAG_OVERLAPPED;
	DWORD dwWriteMode = FILE_FLAG_OVERLAPPED;
	if ((dwReadMode | dwWriteMode) & (~FILE_FLAG_OVERLAPPED)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	//
	//  Set the default timeout to 120 seconds
	//

	if (nSize == 0) {
		nSize = 4096;
	}

	sprintf(PipeNameBuffer,
		"\\\\.\\Pipe\\adb_be_pipe.%08x.%08x",
		GetCurrentProcessId(),
		InterlockedIncrement(&PipeSerialNumber)
		);

	ReadPipeHandle = CreateNamedPipeA(
		PipeNameBuffer,
		PIPE_ACCESS_INBOUND | dwReadMode,
		PIPE_TYPE_BYTE | PIPE_WAIT,
		1,             // Number of pipes
		nSize,         // Out buffer size
		nSize,         // In buffer size
		120 * 1000,    // Timeout in ms
		lpPipeAttributes
		);

	if (!ReadPipeHandle) {
		return FALSE;
	}

	WritePipeHandle = CreateFileA(
		PipeNameBuffer,
		GENERIC_WRITE,
		0,                         // No sharing
		lpPipeAttributes,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | dwWriteMode,
		NULL                       // Template file
		);

	if (INVALID_HANDLE_VALUE == WritePipeHandle) {
		dwError = GetLastError();
		CloseHandle(ReadPipeHandle);
		SetLastError(dwError);
		return FALSE;
	}

	*lpReadPipe = ReadPipeHandle;
	*lpWritePipe = WritePipeHandle;
	return(TRUE);
}

typedef struct adb_backend_data {
    const struct plug_function_table *fn;
    void* frontend;
    /* the above field _must_ be first in the structure */
	HANDLE child_stdin_read, child_stdin_write;
	HANDLE child_stdout_read, child_stdout_write;

	Conf *conf;
} *Adb;

static void adb_size(void *handle, int width, int height);

static void c_write(Adb adb, char *buf, int len)
{
    int backlog = from_backend(adb->frontend, 0, buf, len);
}

static void adb_log(Plug plug, int type, SockAddr addr, int port,
		    const char *error_msg, int error_code)
{
    Adb adb = (Adb) plug;

    //logevent(adb->frontend, msg);
}

static void adb_check_close(Adb adb)
{
    notify_remote_exit(adb->frontend);
}

static int adb_closing(Plug plug, const char *error_msg, int error_code,
		       int calling_back)
{
    Adb adb = (Adb) plug;

    if (error_msg) {
        /* A socket error has occurred. */
    } else {
        /* Otherwise, the remote side closed the connection normally. */
        adb_check_close(adb);
    }
    return 0;
}

static int adb_receive(Plug plug, int urgent, char *data, int len)
{
    Adb adb = (Adb) plug;
    c_write(adb, data, len);
    return 1;
}

static void adb_sent(Plug plug, int bufsize)
{
    Adb adb = (Adb) plug;
}

static char* init_adb_connection(Adb adb)
{
	SECURITY_ATTRIBUTES   sa;
	STARTUPINFO           startup;
	PROCESS_INFORMATION   pinfo;
	char                  program_path[MAX_PATH];
	int                   ret;

	ZeroMemory(&sa, sizeof(sa));
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	/* create pipe, and ensure its read handle isn't inheritable */
	ret = MyCreatePipeEx(&adb->child_stdin_read, &adb->child_stdin_write, &sa, 0);
	if (!ret) { return "CreatePipe() failure"; }
	SetHandleInformation(adb->child_stdin_read, HANDLE_FLAG_INHERIT, 0);

	ret = MyCreatePipeEx(&adb->child_stdout_read, &adb->child_stdout_write, &sa, 0);
	if (!ret) { return "CreatePipe() failure"; }
	SetHandleInformation(adb->child_stdout_write, HANDLE_FLAG_INHERIT, 0);

	ZeroMemory(&startup, sizeof(startup));
	startup.cb = sizeof(startup);
	startup.hStdInput = adb->child_stdin_read;
	startup.hStdOutput = adb->child_stdout_write;
	startup.hStdError = adb->child_stdout_write;// GetStdHandle(STD_ERROR_HANDLE);
	startup.dwFlags = STARTF_USESTDHANDLES;

	ZeroMemory(&pinfo, sizeof(pinfo));

	/* get path of current program */
	GetModuleFileName(NULL, program_path, sizeof(program_path));
	char * ch = strrchr(program_path, '\\');
	//if (ch){ ch++; strcpy(ch, "adb.exe devices"); }
	if (ch){ ch++; strcpy(ch, "adb.exe devices"); }

	ret = CreateProcess(
		NULL,                              /* program path  */
		program_path,
		/* the fork-server argument will set the
		debug = 2 in the child           */
		NULL,                   /* process handle is not inheritable */
		NULL,                    /* thread handle is not inheritable */
		TRUE,                          /* yes, inherit some handles */
		DETACHED_PROCESS, /* the new process doesn't have a console */
		NULL,                     /* use parent's environment block */
		NULL,                    /* use parent's starting directory */
		&startup,                 /* startup info, i.e. std handles */
		&pinfo);

	CloseHandle(adb->child_stdin_read);
	CloseHandle(adb->child_stdout_write);

	if (!ret) {
		CloseHandle(adb->child_stdin_write);
		CloseHandle(adb->child_stdout_read);
		return "CreateProcess failure";
	}

	CloseHandle(pinfo.hThread);
}

/*
 * Called to set up the adb connection.
 * 
 * Returns an error message, or NULL on success.
 *
 * Also places the canonical host name into `realhost'. It must be
 * freed by the caller.
 */
static const char *adb_init(void *frontend_handle, void **backend_handle,
			    Conf *conf,
			    const char *host, int port, char **realhost,
                            int nodelay, int keepalive)
{
    static const struct plug_function_table fn_table = {
	adb_log,
	adb_closing,
	adb_receive,
	adb_sent
    };
    SockAddr addr;
    const char *err;
    Adb adb;
    int addressfamily;
    char *loghost;

    adb = snew(struct adb_backend_data);
    adb->fn = &fn_table;
    adb->frontend = frontend_handle;

    return NULL;
}

static void adb_free(void *handle)
{
    Adb adb = (Adb) handle;
    sfree(adb);
}

/*
 * Stub routine (we don't have any need to reconfigure this backend).
 */
static void adb_reconfig(void *handle, Conf *conf)
{
}

/*
 * Called to send data down the adb connection.
 */
static int adb_send(void *handle, const char *buf, int len)
{
    Adb adb = (Adb) handle;

    return 0;
}

/*
 * Called to query the current socket sendability status.
 */
static int adb_sendbuffer(void *handle)
{
    Adb adb = (Adb) handle;
    return 0;
}

/*
 * Called to set the size of the window
 */
static void adb_size(void *handle, int width, int height)
{
    /* Do nothing! */
    return;
}

/*
 * Send adb special codes. We only handle outgoing EOF here.
 */
static void adb_special(void *handle, Telnet_Special code)
{
    Adb adb = (Adb) handle;
    if (code == TS_EOF) {
        adb_check_close(adb);
    }
    return;
}

/*
 * Return a list of the special codes that make sense in this
 * protocol.
 */
static const struct telnet_special *adb_get_specials(void *handle)
{
    return NULL;
}

static int adb_connected(void *handle)
{
    Adb adb = (Adb) handle;
	return 0;
}

static int adb_sendok(void *handle)
{
    return 1;
}

static void adb_unthrottle(void *handle, int backlog)
{
    Adb adb = (Adb) handle;
}

static int adb_ldisc(void *handle, int option)
{
    if (option == LD_EDIT || option == LD_ECHO)
	return 1;
    return 0;
}

static void adb_provide_ldisc(void *handle, void *ldisc)
{
    /* This is a stub. */
}

static void adb_provide_logctx(void *handle, void *logctx)
{
    /* This is a stub. */
}

static int adb_exitcode(void *handle)
{
    Adb adb = (Adb) handle;
    return 0;
}

/*
 * cfg_info for Adb does nothing at all.
 */
static int adb_cfg_info(void *handle)
{
    return 0;
}

Backend adb_backend = {
    adb_init,
    adb_free,
    adb_reconfig,
    adb_send,
    adb_sendbuffer,
    adb_size,
    adb_special,
    adb_get_specials,
    adb_connected,
    adb_exitcode,
    adb_sendok,
    adb_ldisc,
    adb_provide_ldisc,
    adb_provide_logctx,
    adb_unthrottle,
    adb_cfg_info,
    NULL /* test_for_upstream */,
    "adb",
    PROT_ADB,
    0
};
