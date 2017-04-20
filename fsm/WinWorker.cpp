#include "WinWorker.h"
#include "Log.h"

#include <assert.h>

using namespace Processor;

ThreadLocalStorage g_threadGroupTotal(free);
ThreadLocalStorage g_threadGroupIndex(free);

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}

#ifdef DEBUG
#include <assert.h>
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)
#endif
//-----------------------------------------------------------------------------
WinWorker::WinWorker()
    : groupTotalM(0)
    , groupIndexM(-1)
    , bufferJobQueueM(25) //(32M/8) jobs Max
    , isToStopM(false)
	, queueCondM(&queueMutexM)
{
    min_heap_ctor(&timerHeapM);	
    timeNowM.tv_sec = 0;
    timeNowM.tv_usec = 0;

#ifdef DEBUG 
    tidM = -1;
#endif
}

//-----------------------------------------------------------------------------

WinWorker::~WinWorker()
{
    min_heap_dtor(&timerHeapM);	
}

//-----------------------------------------------------------------------------

void WinWorker::stop()
{
    isToStopM = true;
	queueCondM.Signal();
}

//-----------------------------------------------------------------------------


int WinWorker::process(IJob* theJob)
{
    bool jobQueueEmpty = false;
    {
        AutoLock lock(queueMutexM);
        //jobQueueEmpty = jobQueueM.empty();
        //jobQueueM.push_back(theJob);
        jobQueueEmpty = bufferJobQueueM.empty();
        unsigned len = bufferJobQueueM.putn((char*)&theJob, sizeof(IJob*));
        assert(len > 0);
    }
    if (jobQueueEmpty)
    {
		queueCondM.Signal();
    }
    return 0;
}

//-----------------------------------------------------------------------------

struct min_heap_item_t* WinWorker::addLocalTimer(
	const struct timeval& theInterval,
	TimeoutFn theCallback,
	void* theArg)
{
#ifdef DEBUG
	extern ThreadLocalStorage g_threadGroupTotal;
	extern ThreadLocalStorage g_threadGroupIndex;
    unsigned threadCount = *static_cast<unsigned*>(g_threadGroupTotal.Get());
    unsigned threadIndex = *static_cast<unsigned*>(g_threadGroupIndex.Get());
    if (threadIndex != groupIndexM || threadCount != threadCount)
    {
        LOG_FATAL("job is handled in wrong thread:" << threadIndex 
                << ", count:" << threadCount
                << ", worker's index:" << groupIndexM
                << ", worker's groupCount:" << groupTotalM);
        assert(false);
    }
    

    if (-1 == tidM)
    {
        tidM = gettid();
    }
    else if (tidM != gettid())
    {
        LOG_FATAL("tid not match pre:" << tidM << "-now:" << gettid());
        assert(false);
    }
#endif

	bool timerHeapEmpty = false;
    //if it is the first one, get the time of now
    //else reuse the one in run()
    if ((timerHeapEmpty = min_heap_empty(&timerHeapM)))
    {
		gettimeofday(&timeNowM, NULL);
    }
    if (1000000 > min_heap_size(&timerHeapM))
    {
        min_heap_reserve(&timerHeapM, 1000000);
    }

	struct min_heap_item_t* timeoutEvt = new min_heap_item_t();
    timeoutEvt->timeout.tv_usec = timeNowM.tv_usec + theInterval.tv_usec;
    timeoutEvt->timeout.tv_sec = timeNowM.tv_sec 
        + theInterval.tv_sec 
        + timeoutEvt->timeout.tv_usec/1000000;
    timeoutEvt->timeout.tv_usec %= 1000000;
	timeoutEvt->callback = theCallback;
	timeoutEvt->arg = theArg;	

    if (-1 == min_heap_push(&timerHeapM, timeoutEvt))
    {
        LOG_FATAL("not enough memory!");
        exit(-1);
    }
	if (timerHeapEmpty)
	{
        queueCondM.Signal();
	}
	return timeoutEvt;
}

//-----------------------------------------------------------------------------

void WinWorker::cancelLocalTimer(struct min_heap_item_t*& theEvent)
{
    min_heap_erase(&timerHeapM, theEvent);
	delete theEvent;
	theEvent = NULL;
}

//-----------------------------------------------------------------------------

void WinWorker::initThreadAttr()
{
    g_threadGroupTotal.Set(new unsigned(groupTotalM));
    g_threadGroupIndex.Set(new unsigned(groupIndexM));
}

//-----------------------------------------------------------------------------

void WinWorker::handleLocalTimer()
{
    if (!min_heap_empty(&timerHeapM))
    {
        gettimeofday(&timeNowM, NULL);
        while(!min_heap_empty(&timerHeapM)) 
        {
			struct min_heap_item_t* topEvent = min_heap_top(&timerHeapM);
			if (item_cmp(&topEvent->timeout, &timeNowM, <= ))
            {
                min_heap_pop(&timerHeapM);
                (topEvent->callback)(topEvent->arg);
                delete topEvent;
            }
            else
            {
                break;
            }
        } 
    }


}

//-----------------------------------------------------------------------------

void WinWorker::run()
{
    initThreadAttr();
    IJob* job;
    while (!isToStopM)
    {
        /*
        {
            boost::unique_lock<boost::mutex> lock(queueMutexM);
            while (jobQueueM.empty())
            {
                queueCondM.wait(lock);
            }

            job = jobQueueM.front();
            jobQueueM.pop_front();
        }
        */
        //handle Job
        if (0 < bufferJobQueueM.getn((char*)&job, sizeof(IJob*)))
        {
            (*job)();
            delete job;
            job = NULL;
        }

        //handle timer
        handleLocalTimer();

		if (!bufferJobQueueM.empty())
		{
			continue;
		}
        else if (!isToStopM && bufferJobQueueM.empty() && !min_heap_empty(&timerHeapM))
        {
            AutoLock queueLock(queueMutexM);
			TimeDelta timeDelta = TimeDelta::FromMilliseconds(10);
			queueCondM.TimedWait(timeDelta);
        }
        else
        {
			AutoLock queueLock(queueMutexM);
            while (!isToStopM && bufferJobQueueM.empty() && min_heap_empty(&timerHeapM))
            {
                queueCondM.Wait(); 
            }
        }
    }
}

