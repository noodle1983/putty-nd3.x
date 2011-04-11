
#ifndef __base_threading_thread_restrictions_h__
#define __base_threading_thread_restrictions_h__

#pragma once

#include "../basic_types.h"

namespace base
{

    // 特定的线程中某些行为可能会被禁止. ThreadRestrictions辅助实现这些规则.
    // 这种规则的例子:
    //
    // *不要堵塞IO(导致线程僵死)
    // *不要访问Singleton/LazyInstance(可能导致崩溃)
    //
    // 下面是如何实现保护工作的:
    //
    // 1) 如果一个线程不允许IO调用, 这样做:
    //      base::ThreadRestrictions::SetIOAllowed(false);
    //    缺省情况, 线程是允许调用IO的.
    //
    // 2) 函数调用访问磁盘时, 需要检查当前线程是否允许:
    //      base::ThreadRestrictions::AssertIOAllowed();
    //
    // ThreadRestrictions在release编译时什么都不做; 只在debug下有效.
    //
    // 风格提示: 你应该在什么地方检查AssertIOAllowed? 最好是即将访问磁盘
    // 的时候, 在底层越靠近越好. 这个准则有助于捕获所有的调用. 比如, 如果
    // 你的函数GoDoSomeBlockingDiskCall()只调用其它函数而不是fopen(), 你
    // 应该在辅助函数中添加AssertIOAllowed检查.

    class ThreadRestrictions
    {
    public:
        // 构造一个ScopedAllowIO, 允许当前线程临时访问IO. 这样做大多数情况
        // 都会导致错误.
        class ScopedAllowIO
        {
        public:
            ScopedAllowIO() { previous_value_ = SetIOAllowed(true); }
            ~ScopedAllowIO() { SetIOAllowed(previous_value_); }
        private:
            // 是否允许访问IO.
            bool previous_value_;

            DISALLOW_COPY_AND_ASSIGN(ScopedAllowIO);
        };

        // 构造一个ScopedAllowSingleton, 允许当前线程临时使用单件. 这样做
        // 大多数情况都会导致错误.
        class ScopedAllowSingleton
        {
        public:
            ScopedAllowSingleton() { previous_value_ = SetSingletonAllowed(true); }
            ~ScopedAllowSingleton() { SetSingletonAllowed(previous_value_); }
        private:
            // 是否允许访问单件.
            bool previous_value_;

            DISALLOW_COPY_AND_ASSIGN(ScopedAllowSingleton);
        };

#ifndef NDEBUG
        // 设置当前线程是否允许IO调用. 线程启动缺省是允许的.
        // 返回调用前的值.
        static bool SetIOAllowed(bool allowed);

        // 检查当前线程是否允许IO调用, 不允许会DCHECK. 在何处进行检查参见
        // 类上面的注释.
        static void AssertIOAllowed();

        // 设置当前线程是否允许使用单件. 返回调用前的值.
        static bool SetSingletonAllowed(bool allowed);

        // 检查当前线程是否允许使用单件, 不允许会DCHECK. 
        static void AssertSingletonAllowed();
#else
        // 在Release版本下, 定义内联的空函数, 这样会被编译器优化掉.
        static bool SetIOAllowed(bool allowed) { return true; }
        static void AssertIOAllowed() {}
        static bool SetSingletonAllowed(bool allowed) { return true; }
        static void AssertSingletonAllowed() {}
#endif

    private:
        DISALLOW_IMPLICIT_CONSTRUCTORS(ThreadRestrictions);
    };

} //namespace base

#endif //__base_threading_thread_restrictions_h__