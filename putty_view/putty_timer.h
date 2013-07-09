#ifndef PUTTY_TIMER_H
#define PUTTY_TIMER_H

#include "base/timer.h"
#include "base/memory/singleton.h"
int run_timers(long anow, long *next);

class PuttyTimer
{
public:
	PuttyTimer(){}
	virtual ~PuttyTimer(){}
	static PuttyTimer* GetInstance(){
		return Singleton<PuttyTimer>::get();
	}
	void start(long ticks){
		timer_.Stop();
		timer_.Start(base::TimeDelta::FromMilliseconds(ticks), this, &PuttyTimer::onTimeout);
		lastTimerIndex_ = ticks;
	}

	void onTimeout(){
		long next;
	    timer_.Stop();
	    if (run_timers(lastTimerIndex_, &next)) {
    		start(next);
	    } else {
	    }
	}

private:
	long lastTimerIndex_;
	base::OneShotTimer<PuttyTimer> timer_;

	friend struct DefaultSingletonTraits<PuttyTimer>;
	DISALLOW_COPY_AND_ASSIGN(PuttyTimer);
};

//typedef Singleton<PuttyTimer> PuttyDefaultTimer;

#endif /* PUTTY_TIMER_H */
