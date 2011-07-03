
#include "task.h"

Task::Task() {}

Task::~Task() {}

CancelableTask::CancelableTask() {}

CancelableTask::~CancelableTask() {}

namespace base
{

    ScopedTaskRunner::ScopedTaskRunner(Task* task) : task_(task) {}

    ScopedTaskRunner::~ScopedTaskRunner()
    {
        if(task_)
        {
            task_->Run();
            delete task_;
        }
    }

    Task* ScopedTaskRunner::Release()
    {
        Task* tmp = task_;
        task_ = NULL;
        return tmp;
    }

} //namespace base