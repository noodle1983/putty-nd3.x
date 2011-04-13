
#ifndef __base_threading_thread_checker_h__
#define __base_threading_thread_checker_h__

#pragma once

#include "../synchronization/lock.h"
#include "platform_thread.h"

// 用于验证类方法调用是否在相同的线程中. 从该类继承, 调用CalledOnValidThread()
// 进行验证.
//
// 类的设计用于保证那些不是线程安全的类能被安全使用. 比如, 一个服务或者一个参数
// 设置单件.
//
// 示例:
//     class MyClass : public NonThreadSafe {
//     public:
//       void Foo() {
//         DCHECK(CalledOnValidThread());
//         ... (do stuff) ...
//       }
//     }
//
// 在Release模式下, CalledOnValidThread始终返回true.

#ifndef NDEBUG
class ThreadChecker
{
public:
    ThreadChecker();
    ~ThreadChecker();

    bool CalledOnValidThread() const;

    // 修改CalledOnValidThread的验证线程, 下一次调用CalledOnValidThread会把类附加
    // 到一个新的线程, 是否想暴露此功能取决于NonThreadSafe的派生类. 当一个对象在
    // 一个线程中创建而只在另一个线程中被使用的时候, 可以用的此方法.
    void DetachFromThread();

private:
    void EnsureThreadIdAssigned() const;

    mutable base::Lock lock_;
    // 因为CalledOnValidThread会设置该值, 所以使用mutable.
    mutable base::PlatformThreadId valid_thread_id_;
};
#else
// release模式下什么都不做.
class ThreadChecker
{
public:
    bool CalledOnValidThread() const
    {
        return true;
    }

    void DetachFromThread() {}
};
#endif //NDEBUG

#endif //__base_threading_thread_checker_h__