#include "window_interface.h"

#include "../fsm/WinProcessor.h"

void WindowInterface::at_exit()
{
	g_bg_processor->stop();
	std::map < void*, std::function<void()>> temp_map;
	{
		AutoLock lock(at_exit_map_mutex_);
		temp_map = at_exit_map_;
		at_exit_map_.clear();
	}
	std::map < void*, std::function<void()>>::iterator it = temp_map.begin();
	for (; it != temp_map.end(); it++)
	{
		std::function<void()> cb = it->second;
		cb();
	}

}