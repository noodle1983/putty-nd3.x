
#include "task_queue.h"

#include "base/stl_utilinl.h"

TaskQueue::TaskQueue() {}

TaskQueue::~TaskQueue()
{
    // 拥有指针的所有权, 负责删除指针.
    STLDeleteElements(&queue_);
}

void TaskQueue::Run()
{
    // 如果队列为空直接返回.
    if(queue_.empty())
    {
        return;
    }

    std::deque<Task*> ready;
    queue_.swap(ready);

    // 执行已有的任务.
    std::deque<Task*>::const_iterator task;
    for(task=ready.begin(); task!=ready.end(); ++task)
    {
        // 执行并删除任务.
        (*task)->Run();
        delete (*task);
    }
}

void TaskQueue::Push(Task* task)
{
    DCHECK(task);

    // 添加到队尾.
    queue_.push_back(task);
}

void TaskQueue::Clear()
{
    // 删除队列元素的指针并清理容器.
    STLDeleteElements(&queue_);
}

bool TaskQueue::IsEmpty() const
{
    return queue_.empty();
}