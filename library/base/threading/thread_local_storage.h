
#ifndef __base_threading_thread_local_storage_h__
#define __base_threading_thread_local_storage_h__

#pragma once

#include "../basic_types.h"

// 线程局部存储封装, 除了提供API可移植性外不做任何其它事情.
class ThreadLocalStorage
{
public:
    // TLS析构函数原型, 用于线程退出时清理TLS, 'value'是存储于TLS的数据.
    typedef void (*TLSDestructorFunc)(void* value);

    // 表示TLS存取数据的key.
    class Slot
    {
    public:
        explicit Slot(TLSDestructorFunc destructor=NULL);

        // 应该静态构造, 返回未初始化的插槽.
        explicit Slot(base::LinkerInitialized x) {}

        // 设置TLS插槽. 由构造函数调用. 'destructor'是一个函数指针, 用于
        // 线程清理调用. 如果设置NULL, TLS插槽不需要清理操作. 错误返回false.
        bool Initialize(TLSDestructorFunc destructor);

        // 释放先前分配的TLS插槽. 如果有销毁函数, 移除销毁函数
        // 以便剩余线程退出时不再释放数据.
        void Free();

        // 获取存储于TLS插槽中的值. 初始化为0.
        void* Get() const;

        // 存储值'value'到TLS插槽中.
        void Set(void* value);

        bool initialized() const { return initialized_; }

    private:
        bool initialized_;
        int slot_;

        DISALLOW_COPY_AND_ASSIGN(Slot);
    };

    // 线程退出时调用本函数, 调用所有的销毁函数. 内部使用.
    static void ThreadExit();

private:
    // 延迟初始化TLS的函数.
    static void** Initialize();

private:
    // TLS插槽的最大栈值, 目前是固定的. 可静态的增加, 也可以做成动态的.
    static const int kThreadLocalStorageSize = 64;

    static long tls_key_;
    static long tls_max_;
    static TLSDestructorFunc tls_destructors_[kThreadLocalStorageSize];

    DISALLOW_COPY_AND_ASSIGN(ThreadLocalStorage);
};

#endif //__base_threading_thread_local_storage_h__