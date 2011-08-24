
// 当许多对象引用同一对象, 希望共享对象的生命周期不受引用对象限制的时候可以
// 使用弱指针. 换言之, 用于不想引用计数的情况.
//
// Thread-safety notes:
// When you get a WeakPtr (from a WeakPtrFactory or SupportsWeakPtr),
// the WeakPtr becomes bound to the current thread. You may only
// dereference the WeakPtr on that thread. However, it is safe to
// destroy the WeakPtr object on another thread.
// Since a WeakPtr object may be destroyed on a background thread,
// querying WeakPtrFactory's HasWeakPtrs() method can be racy.
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

#ifndef __base_weak_ptr_h__
#define __base_weak_ptr_h__

#pragma once

#include "base/logging.h"
#include "base/threading/thread_checker.h"
#include "ref_counted.h"

namespace base
{
    namespace internal
    {
        // These classes are part of the WeakPtr implementation.
        // DO NOT USE THESE CLASSES DIRECTLY YOURSELF.

        class WeakReference
        {
        public:
            // While Flag is bound to a specific thread, it may be deleted from another
            // via base::WeakPtr::~WeakPtr().
            class Flag : public RefCountedThreadSafe<Flag>
            {
            public:
                Flag();

                void Invalidate();
                bool IsValid() const;

                void DetachFromThread() { thread_checker_.DetachFromThread(); }

            private:
                friend class base::RefCountedThreadSafe<Flag>;

                ~Flag();

                ThreadChecker thread_checker_;
                bool is_valid_;
            };

            WeakReference();
            explicit WeakReference(const Flag* flag);
            ~WeakReference();

            bool is_valid() const;

        private:
            scoped_refptr<const Flag> flag_;
        };

        class WeakReferenceOwner
        {
        public:
            WeakReferenceOwner();
            ~WeakReferenceOwner();

            WeakReference GetRef() const;

            bool HasRefs() const
            {
                return flag_.get() && !flag_->HasOneRef();
            }

            void Invalidate();

            // Indicates that this object will be used on another thread from now on.
            void DetachFromThread()
            {
                if(flag_) flag_->DetachFromThread();
            }

        private:
            mutable scoped_refptr<WeakReference::Flag> flag_;
        };

        // This class simplifies the implementation of WeakPtr's type conversion
        // constructor by avoiding the need for a public accessor for ref_.  A
        // WeakPtr<T> cannot access the private members of WeakPtr<U>, so this
        // base class gives us a way to access ref_ in a protected fashion.
        class WeakPtrBase
        {
        public:
            WeakPtrBase();
            ~WeakPtrBase();

        protected:
            WeakPtrBase(const WeakReference& ref);

            WeakReference ref_;
        };

    } //namespace internal

    template<typename T> class SupportsWeakPtr;
    template<typename T> class WeakPtrFactory;

    // The WeakPtr class holds a weak reference to |T*|.
    //
    // This class is designed to be used like a normal pointer.  You should always
    // null-test an object of this class before using it or invoking a method that
    // may result in the underlying object being destroyed.
    //
    // EXAMPLE:
    //
    //   class Foo { ... };
    //   WeakPtr<Foo> foo;
    //   if(foo) foo->method();
    template<typename T>
    class WeakPtr : public internal::WeakPtrBase
    {
    public:
        WeakPtr() : ptr_(NULL) {}

        // Allow conversion from U to T provided U "is a" T.
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
            ref_ = internal::WeakReference();
            ptr_ = NULL;
        }

    private:
        friend class SupportsWeakPtr<T>;
        friend class WeakPtrFactory<T>;

        WeakPtr(const internal::WeakReference& ref, T* ptr)
            : WeakPtrBase(ref), ptr_(ptr) {}

        // This pointer is only valid when ref_.is_valid() is true.  Otherwise, its
        // value is undefined (as opposed to NULL).
        T* ptr_;
    };

    // A class may extend from SupportsWeakPtr to expose weak pointers to itself.
    // This is useful in cases where you want others to be able to get a weak
    // pointer to your class.  It also has the property that you don't need to
    // initialize it from your constructor.
    template<class T>
    class SupportsWeakPtr
    {
    public:
        SupportsWeakPtr() {}

        WeakPtr<T> AsWeakPtr()
        {
            return WeakPtr<T>(weak_reference_owner_.GetRef(), static_cast<T*>(this));
        }

        // Indicates that this object will be used on another thread from now on.
        void DetachFromThread()
        {
            weak_reference_owner_.DetachFromThread();
        }

    private:
        internal::WeakReferenceOwner weak_reference_owner_;
        DISALLOW_COPY_AND_ASSIGN(SupportsWeakPtr);
    };

    // A class may alternatively be composed of a WeakPtrFactory and thereby
    // control how it exposes weak pointers to itself.  This is helpful if you only
    // need weak pointers within the implementation of a class.  This class is also
    // useful when working with primitive types.  For example, you could have a
    // WeakPtrFactory<bool> that is used to pass around a weak reference to a bool.
    template<class T>
    class WeakPtrFactory
    {
    public:
        explicit WeakPtrFactory(T* ptr) : ptr_(ptr) {}

        WeakPtr<T> GetWeakPtr()
        {
            return WeakPtr<T>(weak_reference_owner_.GetRef(), ptr_);
        }

        // Call this method to invalidate all existing weak pointers.
        void InvalidateWeakPtrs()
        {
            weak_reference_owner_.Invalidate();
        }

        // Call this method to determine if any weak pointers exist.
        bool HasWeakPtrs() const
        {
            return weak_reference_owner_.HasRefs();
        }

        // Indicates that this object will be used on another thread from now on.
        void DetachFromThread()
        {
            weak_reference_owner_.DetachFromThread();
        }

    private:
        internal::WeakReferenceOwner weak_reference_owner_;
        T* ptr_;
        DISALLOW_IMPLICIT_CONSTRUCTORS(WeakPtrFactory);
    };

} //namespace base

#endif //__base_weak_ptr_h__