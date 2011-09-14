
#ifndef __base_at_exit_h__
#define __base_at_exit_h__

#pragma once

#include <stack>

#include "callback.h"
#include "synchronization/lock.h"

namespace base
{

    // AtExitManager提供了类似CRT的atexit()的功能, 不同的是这里控制了何时调用
    // 回调函数.在Windows下当DLL发生严重错误的时候, 会加锁并调用所有回调函数.
    // AtExitManager一般用在Singleton中.
    //
    // 使用很简单, 只需要在main()或者WinMain()函数作用域开头创建一个栈对象:
    //     int main(...)
    //     {
    //         base::AtExitManager exit_manager;
    //     }
    // 当exit_manager离开作用域的时候, 所有注册的回调函数和单件的析构函数
    // 会被调用.
    class AtExitManager
    {
    public:
        typedef void (*AtExitCallbackType)(void*);

        AtExitManager();
        // 析构函数调用所有注册的回调, 析构之后不用再注册回调.
        ~AtExitManager();

        // 注册退出时的回调函数. 函数原型是void func(void*).
        static void RegisterCallback(AtExitCallbackType func, void* param);

        // Registers the specified task to be called at exit.
        static void RegisterTask(base::Closure task);

        // 以LIFO序调用注册的回调函数, 函数返回之后可以注册新的回调.
        static void ProcessCallbacksNow();

    private:
        Lock lock_;
        std::stack<base::Closure> stack_;

        DISALLOW_COPY_AND_ASSIGN(AtExitManager);
    };

} //namespace base

#endif //__base_at_exit_h__