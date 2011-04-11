
#ifndef __message_framework_task_queue_h__
#define __message_framework_task_queue_h__

#pragma once

#include <deque>

#include "task.h"

// TaskQueue是等待执行的任务队列. 执行任务调用Run方法. TaskQueue也是任务, 所以
// 可以放置到消息循环或者其它消息队列.
class TaskQueue : public Task
{
public:
    TaskQueue();
    ~TaskQueue();

    // 执行队列中所有的任务. 执行过程中添加的新任务将会在下一次|Run|调用时执行.
    virtual void Run();

    // 添加任务到队列, 队列执行的时候按照任务添加的顺序依次执行.
    //
    // 接管|task|所有权, 任务运行完成后会被自动删除(没被执行的任务会在TaskQueue析构
    // 时销毁).
    void Push(Task* task);

    // 移除队列中的所有任务, 任务被删除.
    void Clear();

    // 如果队列中没有任务返回true.
    bool IsEmpty() const;

private:
    // 等待执行的任务.
    std::deque<Task*> queue_;
};

#endif //__message_framework_task_queue_h__