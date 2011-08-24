
#include "at_exit.h"

#include "logging.h"

namespace base
{

    static AtExitManager* g_top_manager = NULL;

    AtExitManager::AtExitManager()
    {
        DCHECK(!g_top_manager);
        g_top_manager = this;
    }

    AtExitManager::~AtExitManager()
    {
        if(!g_top_manager)
        {
            NOTREACHED() << "Tried to ~AtExitManager without an AtExitManager";
            return;
        }
        DCHECK(g_top_manager == this);

        ProcessCallbacksNow();
        g_top_manager = NULL;
    }

    void AtExitManager::RegisterCallback(AtExitCallbackType func, void* param)
    {
        if(!g_top_manager)
        {
            NOTREACHED() << "Tried to RegisterCallback without an AtExitManager";
            return;
        }

        DCHECK(func);

        AutoLock lock(g_top_manager->lock_);
        g_top_manager->stack_.push(CallbackAndParam(func, param));
    }

    void AtExitManager::ProcessCallbacksNow()
    {
        if(!g_top_manager)
        {
            NOTREACHED() << "Tried to ProcessCallbacksNow without an AtExitManager";
            return;
        }

        AutoLock lock(g_top_manager->lock_);

        while(!g_top_manager->stack_.empty())
        {
            CallbackAndParam callback_and_param = g_top_manager->stack_.top();
            g_top_manager->stack_.pop();

            callback_and_param.func_(callback_and_param.param_);
        }
    }

} //namespace base