#ifndef WIN_MUTEX_H
#define WIN_MUTEX_H


#include <base/synchronization/lock.h>
#include <base/synchronization/condition_variable.h>
#include <base/threading/thread_local_storage.h>
#include <base/time.h>
#include <base/threading/platform_thread.h>
#include <boost/function.hpp>

typedef base::Lock Lock;
typedef base::AutoLock AutoLock;
typedef base::ConditionVariable ConditionVariable;
typedef base::ThreadLocalStorage::Slot ThreadLocalStorage;
typedef base::TimeDelta TimeDelta;
typedef base::PlatformThread PlatformThread;
typedef base::PlatformThreadHandle PlatformThreadHandle;
#define joinThread PlatformThread::Join

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	// FILETIME Jan 1 1970 00:00:00
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  nSystemTime;
	FILETIME    nFileTime;
	uint64_t    nTime;

	GetSystemTime(&nSystemTime);
	SystemTimeToFileTime(&nSystemTime, &nFileTime);
	nTime = ((uint64_t)nFileTime.dwLowDateTime);
	nTime += ((uint64_t)nFileTime.dwHighDateTime) << 32;

	tp->tv_sec = (long)((nTime - EPOCH) / 10000000L);
	tp->tv_usec = (long)(nSystemTime.wMilliseconds * 1000);
	return 0;
}



#endif