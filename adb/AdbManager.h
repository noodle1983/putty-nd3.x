#ifndef ADBMANAGER_H
#define ADBMANAGER_H

#include "../fsm/WinProcessor.h"

#define g_adbm_processor (DesignPattern::Singleton<Processor::WinProcessor, 0>::instance())

class AdbManager
{
public:
	AdbManager();
	virtual ~AdbManager();
	void scan();
	void stop_scan();

private:
	void internal_start_scan();
	static void internal_scan_timeout(void* arg);

private:
	struct min_heap_item_t* mLocalTimer;
	bool mShouldScan;
};

#define g_adb_manager (DesignPattern::Singleton<AdbManager, 0>::instance())

#endif
