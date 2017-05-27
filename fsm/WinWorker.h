#ifndef WINWORKER_H
#define WINWORKER_H

#include <list>
#include "boost/function.hpp"
#include "boost/bind.hpp"
#include "min_heap.h"
#include "KfifoBuffer.h"
#include "WinMutex.h"
#include "Singleton.hpp"

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
		int process(const unsigned long long theId, IJob* theJob);
		struct min_heap_item_t* addLocalTimer(
				const struct timeval& theInterval, 
				TimeoutFn theCallback,
				void* theArg);
		void cancelLocalTimer(struct min_heap_item_t*& theEvent);

        void run();
		void handle_jobs();

        //for timer only
        void initThreadAttr();
		void handleLocalTimer();
        // and addLocalTimer cancelLocalTimer

		void process_ui_jobs();
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
#define g_ui_processor (DesignPattern::Singleton<Processor::WinWorker, 0>::instance())

#endif /* WINWORKER_H */

