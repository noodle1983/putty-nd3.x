
// 当许多对象引用同一对象, 希望共享对象的生命周期不受引用对象限制的时候可以
// 使用弱指针. 换言之, 用于不想引用计数的情况.
//
// 弱指针的另外一种选择就是让共享对象拥有所有引用对象, 在共享对象析构的时候
// 调用所有引用对象方法以丢弃引用. 引用对象析构的时候也需要通知共享对象把自
// 己从引用队列中移除. 这种方案可行, 但有一些复杂.
//
// 示例:
//
//    class Controller : public SupportsWeakPtr<Controller> {
//      public:
//      void SpawnWorker() { Worker::StartNew(AsWeakPtr()); }
//      void WorkComplete(const Result& result) { ... }
//    };
//
//    class Worker {
//    public:
//      static void StartNew(const WeakPtr<Controller>& controller) {
//        Worker* worker = new Worker(controller);
//        // Kick off asynchronous processing...
//      }
//    private:
//      Worker(const WeakPtr<Controller>& controller)
//        : controller_(controller) {}
//      void DidCompleteAsynchronousProcessing(const Result& result) {
//        if(controller_)
//          controller_->WorkComplete(result);
//      }
//      WeakPtr<Controller> controller_;
//    };
//
// 对于上面的类, 用户创建一个Controller对象, 调用多次SpawnWorker, 然后在所有
// 工作完成之前销毁Controller对象. 由于Worker类只是拥有Controller的弱指针,
// Controller销毁以后弱指针的解引用不会出现野指针.
//
// 警告: 弱指针不是线程安全的!!! WeakPtr对象只能在创建的线程中使用.

#ifndef __base_weak_ptr_h__
#define __base_weak_ptr_h__

#pragma once

#include "logging.h"
#include "ref_counted.h"
#include "threading/non_thread_safe.h"

namespace base
{

    // 这些类是WeakPtr的实现部分, 不要直接使用.
    class WeakReference
    {
    public:
        class Flag : public RefCounted<Flag>, public NonThreadSafe
        {
        public:
            Flag(Flag** handle);
            ~Flag();

            void AddRef() const;
            void Release() const;
            void Invalidate() { handle_ = NULL; }
            bool is_valid() const { return handle_ != NULL; }

            void DetachFromThread() { NonThreadSafe::DetachFromThread(); }

        private:
            Flag** handle_;
        };

        WeakReference();
        WeakReference(Flag* flag);
        ~WeakReference();

        bool is_valid() const;

    private:
        scoped_refptr<Flag> flag_;
    };

    class WeakReferenceOwner
    {
    public:
        WeakReferenceOwner();
        ~WeakReferenceOwner();

        WeakReference GetRef() const;

        bool HasRefs() const
        {
            return flag_ != NULL;
        }

        void Invalidate();

        // 表明对象从现在起在另一个线程中使用.
        void DetachFromThread()
        {
            if(flag_) flag_->DetachFromThread();
        }

    private:
        mutable WeakReference::Flag* flag_;
    };

    // 简化不同类型的WeakPtr拷贝构造, 避免访问ref_导致其public. WeakPtr<T>
    // 不能直接访问WeakPtr<U>的私有成员, 故基类中ref_的访问控制域为受保护的.
    class WeakPtrBase
    {
    public:
        WeakPtrBase();
        ~WeakPtrBase();

    protected:
        WeakPtrBase(const WeakReference& ref);

        WeakReference ref_;
    };

    template<typename T> class SupportsWeakPtr;
    template<typename T> class WeakPtrFactory;

    // WeakPtr类拥有|T*|的弱引用.
    //
    // 类使用起来就像一般的指针, 调用对象成员方法前始终需要判断是否为空, 底层
    // 对象的销毁会导致弱引用指针无效.
    //
    // 示例:
    //
    //     class Foo { ... };
    //     WeakPtr<Foo> foo;
    //     if(foo)
    //       foo->method();
    template<typename T>
    class WeakPtr : public WeakPtrBase
    {
    public:
        WeakPtr() : ptr_(NULL) {}

        // 如果U是T, 允许从U到T的类型转换.
        template<typename U>
        WeakPtr(const WeakPtr<U>& other) : WeakPtrBase(other), ptr_(other.get()) {}

        T* get() const { return ref_.is_valid() ? ptr_ : NULL; }
        operator T*() const { return get(); }

        T* operator*() const
        {
            DCHECK(get() != NULL);
            return *get();
        }
        T* operator->() const
        {
            DCHECK(get() != NULL);
            return get();
        }

        void reset()
        {
            ref_ = WeakReference();
            ptr_ = NULL;
        }

    private:
        friend class SupportsWeakPtr<T>;
        friend class WeakPtrFactory<T>;

        WeakPtr(const WeakReference& ref, T* ptr)
            : WeakPtrBase(ref), ptr_(ptr) {}

        // 指针只有在ref_.is_valid()为true的时候合法, 否则值是未定义的(不是NULL).
        T* ptr_;
    };

    // 派生自SupportsWeakPtr的类, 可以暴露自己的弱指针. 当想要给其它对象自己的弱
    // 指针时, 类可以从SupportsWeakPtr派生, 在你的构造函数中不需要初始化它.
    template<class T>
    class SupportsWeakPtr
    {
    public:
        SupportsWeakPtr() {}

        WeakPtr<T> AsWeakPtr()
        {
            return WeakPtr<T>(weak_reference_owner_.GetRef(), static_cast<T*>(this));
        }

        // 表明对象从现在起在另一个线程中使用.
        void DetachFromThread()
        {
            weak_reference_owner_.DetachFromThread();
        }

    private:
        WeakReferenceOwner weak_reference_owner_;
        DISALLOW_COPY_AND_ASSIGN(SupportsWeakPtr);
    };

    // 类可以选择组合WeakPtrFactory用于控制是否暴露自己的弱指针, 类内部实现需要
    // 弱指针的时候非常有用. 类可以和原始类型配合使用, 比如可以用WeakPtrFactory<bool>
    // 传一个bool的递弱引用.
    template<class T>
    class WeakPtrFactory
    {
    public:
        explicit WeakPtrFactory(T* ptr) : ptr_(ptr) {}

        WeakPtr<T> GetWeakPtr()
        {
            return WeakPtr<T>(weak_reference_owner_.GetRef(), ptr_);
        }

        // 方法使所有弱指针非法.
        void InvalidateWeakPtrs()
        {
            weak_reference_owner_.Invalidate();
        }

        // 判断是否存在弱指针.
        bool HasWeakPtrs() const
        {
            return weak_reference_owner_.HasRefs();
        }

    private:
        WeakReferenceOwner weak_reference_owner_;
        T* ptr_;

        DISALLOW_IMPLICIT_CONSTRUCTORS(WeakPtrFactory);
    };

} //namespace base

#endif //__base_weak_ptr_h__