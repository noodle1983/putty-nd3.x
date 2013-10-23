#ifndef FSMINTERFACE_H
#define FSMINTERFACE_H

#define NOMINMAX

#include <boost/bind.hpp> 
#include <boost/function.hpp>
#define FSM_BIND boost::bind
#define FSM_FUNCTION boost::function

//thread related
#include <base/callback.h>
#include "base/message_loop.h"
#include <base/task.h>
class FsmTask : public Task
{
public:
    FsmTask(FSM_FUNCTION<void(void)> func)
		: func_(func)
	{}
	virtual ~FsmTask(){}

    // 不是所有的任务都支持取消操作.
	virtual void Run(){func_();}

private:
	std::function<void(void)> func_;
};
#define ASYN_PROCESS_EVT(...) MessageLoopForUI::current()->PostTask(new FsmTask(FSM_BIND(__VA_ARGS__)))

//timer
#include "base/timer.h"
template <typename CallbackClass>
class FsmTimer
{
public:
	FsmTimer(CallbackClass* obj)
		: obj_(obj)
	{}
	virtual ~FsmTimer(){}

	void start(long ms){
		timer_.Start(base::TimeDelta::FromMilliseconds(ms), this, &FsmTimer::onTimeout);
	}

	void onTimeout(){
	    timer_.Stop();
	    obj_->handleTimeout();
	}

	void cancelTimer(){
		timer_.Stop();
	}

private:
	CallbackClass* obj_;
	base::OneShotTimer<FsmTimer> timer_;

	DISALLOW_COPY_AND_ASSIGN(FsmTimer);
};

//log relate
#define LOG_TRACE(msg) 
#define LOG_DEBUG(msg) 
#define LOG_INFO(msg) 
#define LOG_WARN(msg) 
#define LOG_ERROR(msg) 
#define LOG_FATAL(msg) 

#endif
