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

extern "C" int gettimeofday(struct timeval * tp, struct timezone * tzp);



#endif