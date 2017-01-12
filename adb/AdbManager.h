#ifndef ADBMANAGER_H
#define ADBMANAGER_H

#include "../fsm/WinProcessor.h"

#include<map>
#include<string>

#define g_adbm_processor (DesignPattern::Singleton<Processor::WinProcessor, 0>::instance())
typedef std::map<std::string, std::string> DeviceMapType;

class AdbManager
{
public:
	AdbManager();
	virtual ~AdbManager();
	void scan();
	void stop_scan();

	void update_device();

private:
	void internal_start_scan();
	static void internal_scan_timeout(void* arg);

	void parse_device(const char* deviceStr);

private:
	struct min_heap_item_t* mLocalTimer;
	bool mShouldScan;

	Lock mDeviceMutexM;
	DeviceMapType mDeviceMap;
};

#define g_adb_manager (DesignPattern::Singleton<AdbManager, 0>::instance())

#endif
