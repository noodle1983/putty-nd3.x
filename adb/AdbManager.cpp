#include "AdbManager.h"
#include "sysdeps.h"
extern "C"
{
#include "adb_client.h"
}
#pragma comment(lib, "ws2_32.lib")

void start_adb_scan()
{
	g_adb_manager->scan();
}

void stop_adb_scan()
{
	g_adb_manager->stop_scan();
}

extern "C" void adb_sysdeps_init2();
AdbManager::AdbManager()
	: mLocalTimer(NULL)
	, mShouldScan(FALSE)
{
	adb_sysdeps_init2();
	adb_trace_init();
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
	if (g_adb_manager->mShouldScan)
	{
		char *tmp = adb_query("host:devices");
		if (tmp) {
			printf("List of devices attached \n");
			printf("%s\n", tmp);
			free(tmp);
		}
	}
	g_adb_manager->internal_start_scan();
}