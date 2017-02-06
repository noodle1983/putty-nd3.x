#include "AdbManager.h"
#include "FsmInterface.h"
#include <sstream>
#include <stdio.h>

/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples.
*       Copyright 1995 - 1997 Microsoft Corporation.
*       All rights reserved.
*       This source code is only intended as a supplement to
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the
*       Microsoft samples programs.
\******************************************************************************/

/*++
Copyright (c) 1997  Microsoft Corporation
Module Name:
pipeex.c
Abstract:
CreatePipe-like function that lets one or both handles be overlapped
Author:
Dave Hart  Summer 1997
Revision History:
--*/

#include <windows.h>
#include <stdio.h>

ULONG PipeSerialNumber = 0;

BOOL
APIENTRY
MyCreatePipeEx(
OUT LPHANDLE lpReadPipe,
OUT LPHANDLE lpWritePipe,
IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
IN DWORD nSize
)

/*++
Routine Description:
The CreatePipeEx API is used to create an anonymous pipe I/O device.
Unlike CreatePipe FILE_FLAG_OVERLAPPED may be specified for one or
both handles.
Two handles to the device are created.  One handle is opened for
reading and the other is opened for writing.  These handles may be
used in subsequent calls to ReadFile and WriteFile to transmit data
through the pipe.
Arguments:
lpReadPipe - Returns a handle to the read side of the pipe.  Data
may be read from the pipe by specifying this handle value in a
subsequent call to ReadFile.
lpWritePipe - Returns a handle to the write side of the pipe.  Data
may be written to the pipe by specifying this handle value in a
subsequent call to WriteFile.
lpPipeAttributes - An optional parameter that may be used to specify
the attributes of the new pipe.  If the parameter is not
specified, then the pipe is created without a security
descriptor, and the resulting handles are not inherited on
process creation.  Otherwise, the optional security attributes
are used on the pipe, and the inherit handles flag effects both
pipe handles.
nSize - Supplies the requested buffer size for the pipe.  This is
only a suggestion and is used by the operating system to
calculate an appropriate buffering mechanism.  A value of zero
indicates that the system is to choose the default buffering
scheme.
Return Value:
TRUE - The operation was successful.
FALSE/NULL - The operation failed. Extended error status is available
using GetLastError.
--*/

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
		"\\\\.\\Pipe\\adb_pipe.%08x.%08x",
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

void start_adb_scan()
{
	g_adb_manager->scan();
}

void check_update_device()
{
	g_adb_manager->update_device();
}

void stop_adb_scan()
{
	g_adb_manager->stop_scan();
}

AdbManager::AdbManager()
	: mLocalTimer(NULL)
	, mShouldScan(FALSE)
{
}

AdbManager::~AdbManager()
{

}

void AdbManager::scan()
{
	mShouldScan = true;
	g_adbm_processor->process(0, NEW_PROCESSOR_JOB(&AdbManager::internal_start_scan, this));
}


void AdbManager::stop_scan()
{
	mShouldScan = false;
}

void AdbManager::internal_start_scan()
{
	if (mShouldScan && (mLocalTimer == NULL))
	{
		struct timeval timeout;
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;

		mLocalTimer = g_adbm_processor->addLocalTimer(0, timeout, AdbManager::internal_scan_timeout, NULL);
	}
}

void AdbManager::internal_scan_timeout(void* arg)
{
	g_adb_manager->mLocalTimer = NULL;
	//scan
	for (int i = 0; i < 1 && g_adb_manager->mShouldScan; i++)
	{
		HANDLE                pipe_read, pipe_write;
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
		ret = MyCreatePipeEx(&pipe_read, &pipe_write, &sa, 0);
		if (!ret) {
			fprintf(stderr, "CreatePipe() failure, error %ld\n", GetLastError());
			break;
		}

		SetHandleInformation(pipe_read, HANDLE_FLAG_INHERIT, 0);

		ZeroMemory(&startup, sizeof(startup));
		startup.cb = sizeof(startup);
		startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		startup.hStdOutput = pipe_write;
		startup.hStdError = pipe_write;// GetStdHandle(STD_ERROR_HANDLE);
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

		CloseHandle(pipe_write);

		if (!ret) {
			fprintf(stderr, "CreateProcess failure, error %ld\n", GetLastError());
			CloseHandle(pipe_read);
			break;
		}

		CloseHandle(pinfo.hThread);


		char  temp[65536];
		int total_count = 0;
		DWORD  count;
		ULONGLONG start_time = GetTickCount64();
		OVERLAPPED overlapped;
		ZeroMemory(&overlapped, sizeof(OVERLAPPED));
		do{
			count = 0;
			ret = ReadFile(pipe_read, temp + total_count, sizeof(temp)-total_count, &count, &overlapped);
			total_count += count;
			//if (GetTickCount64() - start_time > 10 * 1000){ break; }
			if (!ret) {
				int err = GetLastError();
				if (err == ERROR_IO_PENDING)
				{ 
					WaitForSingleObject(pipe_read, INFINITE);
					GetOverlappedResult(pipe_read, &overlapped, &count, FALSE);
					total_count += count;
					continue; 
				}
				fprintf(stderr, "could not read ok from ADB Server, error = %ld\n", err);
				break;
			}
			else{
				Sleep(100);
			}
		} while (1);
		CloseHandle(pipe_read);
		TerminateProcess(pinfo.hProcess, 0);
		CloseHandle(pinfo.hProcess);
		g_adb_manager->parse_device(temp);
		break;
	}
	g_adb_manager->internal_start_scan();
}

void AdbManager::parse_device(const char* deviceStr)
{
	AutoLock lock(mDeviceMutexM);
	mDeviceMap.clear();

	char line[2048] = { 0 };
	std::stringstream ss(deviceStr);
	ss.getline(line, sizeof(line));
	while (ss.good())
	{
		ss.getline(line, sizeof(line));
		if (!ss.good()){ break; }
		char deviceId[256] = { 0 };
		char deviceType[256] = { 0 };
		int num = sscanf(line, "%s %s", deviceId, deviceType);
		if (num == 2){ mDeviceMap[deviceId] = deviceType; }
	}
}

extern void got_adb_devices(std::map<std::string, std::string> & deviceMap);
void AdbManager::update_device()
{
	if (!mShouldScan){ return; }
	AutoLock lock(mDeviceMutexM);
	if (mDeviceMap.empty()){ return; }

	got_adb_devices(mDeviceMap);
	mDeviceMap.clear();

}