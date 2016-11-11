#include "WinProcessor.h"
#include "WinWorker.h"

#include <boost/bind.hpp>

using namespace Processor;

class ThreadMainDelegate : public PlatformThread::Delegate
{
public:
	ThreadMainDelegate(WinWorker* worker)
		: mWorker(worker)
	{}
	void ThreadMain()
	{
		mWorker->run();
		delete this;
	}
private:
	WinWorker* mWorker;
};

void CreateThread(WinWorker* worker, PlatformThreadHandle* out_thread_handle)
{
	base::PlatformThread::Create(0, new ThreadMainDelegate(worker), out_thread_handle);
}

//-----------------------------------------------------------------------------

WinProcessor::WinProcessor(const unsigned theThreadCount)
    : threadCountM(theThreadCount)
    , workersM(NULL)
	, threadsM(NULL)
{
}

//-----------------------------------------------------------------------------

WinProcessor::WinProcessor(const std::string& theName, const unsigned theThreadCount)
    : threadCountM(theThreadCount)
    , workersM(NULL)
	, threadsM(NULL)
    , nameM(theName)
{
}

//-----------------------------------------------------------------------------

WinProcessor::~WinProcessor()
{
    if (workersM)
    {
        stop();
        delete []workersM;
    }
}

//-----------------------------------------------------------------------------

void WinProcessor::start()
{
    if (0 == threadCountM)
        return;

    if (NULL != workersM)
		return;
	if (NULL != threadsM)
		return;

    workersM = new WinWorker[threadCountM];
	threadsM = new PlatformThreadHandle[threadCountM];
    for (unsigned i = 0; i < threadCountM; i++)
    {
        workersM[i].setGroupInfo(threadCountM, i);
		CreateThread(&workersM[i], &threadsM[i]);
    }
}

//-----------------------------------------------------------------------------

void WinProcessor::waitStop()
{
    if (NULL == workersM)
        return;

    unsigned int i = 0;
    while(true)
    {
        /* check the worker once only */
        if(i < threadCountM && workersM[i].isJobQueueEmpty())
        {
            workersM[i].stop();
			joinThread(threadsM[i]);
            i++;
        }
        if (i == threadCountM)
        {
            break;
        }
        else
        {
            Sleep(1);
        }
    }
}

//-----------------------------------------------------------------------------

void WinProcessor::stop()
{
    if (NULL == workersM)
        return;

    for (unsigned i = 0; i < threadCountM; i++)
    {
		workersM[i].stop();
		joinThread(threadsM[i]);
    }
}

//-----------------------------------------------------------------------------

