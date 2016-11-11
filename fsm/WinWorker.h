#ifndef WINWORKER_H
#define WINWORKER_H

#include <boost/function.hpp>
#include <list>
#include "min_heap.h"
#include "KfifoBuffer.h"
#include "WinMutex.h"

namespace Processor
{
	typedef boost::function<void()> IJob;
    typedef std::list<IJob*> JobQueue;
	typedef void(*TimeoutFn)(void *arg);
    class WinWorker
    {
    public:
        WinWorker();
        ~WinWorker();
        void setGroupInfo(const unsigned theTotal, const unsigned theIndex)
        {
            groupTotalM = theTotal;
            groupIndexM = theIndex;
        }

        inline bool isJobQueueEmpty()
        {return bufferJobQueueM.empty();}
		inline unsigned getQueueSize()
		{return bufferJobQueueM.size()/sizeof(void*);}
        void stop();

        int process(IJob* theJob);
		struct min_heap_item_t* addLocalTimer(
				const struct timeval& theInterval, 
				TimeoutFn theCallback,
				void* theArg);
		void cancelLocalTimer(struct min_heap_item_t*& theEvent);

        void run();

        //for timer only
        void initThreadAttr();
		void handleLocalTimer();
        // and addLocalTimer cancelLocalTimer

    private:
        unsigned groupTotalM;
        unsigned groupIndexM;
		
        JobQueue jobQueueM;
		KfifoBuffer bufferJobQueueM;
		Lock queueMutexM;
		Lock nullMutexM;
		ConditionVariable queueCondM;

		//integrate timer handling
		min_heap_t timerHeapM;
		struct timeval timeNowM;	

        mutable bool isToStopM;
#ifdef DEBUG
		int64_t tidM;
#endif
    };
}

#endif /* WINWORKER_H */

