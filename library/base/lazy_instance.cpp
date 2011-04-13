
#include "lazy_instance.h"

#include "at_exit.h"
#include "threading/platform_thread.h"

namespace base
{

    bool LazyInstanceHelper::NeedsInstance()
    {
        // 尝试创建实例, 如果是第一次到达, state会_从EMPTY变为CREATING.
        // 否则表示被抢先了.
        if(Acquire_CompareAndSwap(&state_, STATE_EMPTY,
            STATE_CREATING) == STATE_EMPTY)
        {
            // 调用者必须创建实例.
            return true;
        }
        else
        {
            // 要么正在创建要么已经创建完成. 循环等待.
            while(NoBarrier_Load(&state_) != STATE_CREATED)
            {
                PlatformThread::YieldCurrentThread();
            }
        }

        // 其它线程已经创建过实例.
        return false;
    }

    void LazyInstanceHelper::CompleteInstance(void* instance, void (*dtor)(void*))
    {
        // 实例被创建, 从CREATING变为CREATED.
        Release_Store(&state_, STATE_CREATED);

        // 确保实例化的对象在程序退出时被析构.
        if(dtor)
        {
            AtExitManager::RegisterCallback(dtor, instance);
        }
    }

} //namespace base