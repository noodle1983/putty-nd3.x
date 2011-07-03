
#ifndef __base_ref_counted_h__
#define __base_ref_counted_h__

#pragma once

#include "base/atomic_ref_count.h"

namespace base
{
    namespace subtle
    {

        class RefCountedBase
        {
        public:
            static bool ImplementsThreadSafeReferenceCounting() { return false; }
            bool HasOneRef() const { return ref_count_ == 1; }

        protected:
            RefCountedBase();
            ~RefCountedBase();

            void AddRef() const;

            // 对象删除自己时返回true.
            bool Release() const;

        private:
            mutable int ref_count_;
#ifndef NDEBUG
            mutable bool in_dtor_;
#endif

            DISALLOW_COPY_AND_ASSIGN(RefCountedBase);
        };

        class RefCountedThreadSafeBase
        {
        public:
            static bool ImplementsThreadSafeReferenceCounting() { return true; }
            bool HasOneRef() const;

        protected:
            RefCountedThreadSafeBase();
            ~RefCountedThreadSafeBase();

            void AddRef() const;

            // 对象删除自己时返回true.
            bool Release() const;

        private:
            mutable AtomicRefCount ref_count_;
#ifndef NDEBUG
            mutable bool in_dtor_;
#endif

            DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafeBase);
        };

    } //namespace subtle

    // 引用计数类的基类. 像这样使用它来进行扩展:
    //
    //     class MyFoo : public base::RefCounted<MyFoo> {
    //      ...
    //      private:
    //       friend class base::RefCounted<MyFoo>;
    //       ~MyFoo();
    //     };
    //
    // 析构函数始终是private的, 避免不小心直接调用delete操作.
    template<class T>
    class RefCounted : public subtle::RefCountedBase
    {
    public:
        RefCounted() {}
        ~RefCounted() {}

        void AddRef() const
        {
            subtle::RefCountedBase::AddRef();
        }

        void Release() const
        {
            if(subtle::RefCountedBase::Release())
            {
                delete static_cast<const T*>(this);
            }
        }

    private:
        DISALLOW_COPY_AND_ASSIGN(RefCounted<T>);
    };

    template<class T, typename Traits> class RefCountedThreadSafe;

    // RefCountedThreadSafe<T>缺省的特性. 当引用计数为0时删除对象.
    // 重载可以实现在其它线程删除对象等.
    template<typename T>
    struct DefaultRefCountedThreadSafeTraits
    {
        static void Destruct(T* x)
        {
            // 通过RefCountedThreadSafe删除, 这样子类只需要声明RefCountedThreadSafe
            // 为友元, 而不是这个结构体.
            RefCountedThreadSafe<T,
                DefaultRefCountedThreadSafeTraits>::DeleteInternal(x);
        }
    };

    // A thread-safe variant of RefCounted<T>
    //
    //   class MyFoo : public base::RefCountedThreadSafe<MyFoo> {
    //    ...
    //   };
    //
    // If you're using the default trait, then you should add compile time
    // asserts that no one else is deleting your object.  i.e.
    //    private:
    //     friend class base::RefCountedThreadSafe<MyFoo>;
    //     ~MyFoo();
    template<class T, typename Traits=DefaultRefCountedThreadSafeTraits<T> >
    class RefCountedThreadSafe : public subtle::RefCountedThreadSafeBase
    {
    public:
        RefCountedThreadSafe() {}
        ~RefCountedThreadSafe() {}

        void AddRef()
        {
            subtle::RefCountedThreadSafeBase::AddRef();
        }

        void Release()
        {
            if(subtle::RefCountedThreadSafeBase::Release())
            {
                Traits::Destruct(static_cast<T*>(this));
            }
        }

    private:
        friend struct DefaultRefCountedThreadSafeTraits<T>;
        static void DeleteInternal(T* x) { delete x; }

        DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafe);
    };

    // 封装一些数据用于实现scoped_refptrs<>.
    template<typename T>
    class RefCountedData : public base::RefCounted< base::RefCountedData<T> >
    {
    public:
        RefCountedData() : data() {}
        RefCountedData(const T& in_value) : data(in_value) {}

        T data;
    };

} //namespace base

// 智能指针用于对象的引用计数. 使用这个类不需要手动的调用AddRef和Release, 可以
// 避免因忘记调用Release导致的内存泄漏. 用法示例:
//
//     class MyFoo : public RefCounted<MyFoo> {
//      ...
//     };
//
//     void some_function() {
//       scoped_refptr<MyFoo> foo = new MyFoo();
//       foo->Method(param);
//       // |foo| is released when this function returns
//     }
//
//     void some_other_function() {
//       scoped_refptr<MyFoo> foo = new MyFoo();
//       ...
//       foo = NULL;  // explicitly releases |foo|
//       ...
//       if(foo)
//         foo->Method(param);
//     }
//
// 上面的例子展示了scoped_refptr<T>的行为就像一个指针, 两个scoped_refptr<T>
// 对象可以互换引用, 比如:
//
//     {
//       scoped_refptr<MyFoo> a = new MyFoo();
//       scoped_refptr<MyFoo> b;
//
//       b.swap(a);
//       // now, |b| references the MyFoo object, and |a| references NULL.
//     }
//
// 要是|a|和|b|引用同一个MyFoo对象, 可以直接使用赋值操作:
//
//     {
//       scoped_refptr<MyFoo> a = new MyFoo();
//       scoped_refptr<MyFoo> b;
//
//       b = a;
//       // now, |a| and |b| each own a reference to the same MyFoo object.
//     }
template<class T>
class scoped_refptr
{
public:
    scoped_refptr() : ptr_(NULL) {}

    scoped_refptr(T* p) : ptr_(p)
    {
        if(ptr_)
        {
            ptr_->AddRef();
        }
    }

    scoped_refptr(const scoped_refptr<T>& r) : ptr_(r.ptr_)
    {
        if(ptr_)
        {
            ptr_->AddRef();
        }
    }

    template<typename U>
    scoped_refptr(const scoped_refptr<U>& r) : ptr_(r.get())
    {
        if(ptr_)
        {
            ptr_->AddRef();
        }
    }

    ~scoped_refptr()
    {
        if(ptr_)
        {
            ptr_->Release();
        }
    }

    T* get() const { return ptr_; }
    operator T*() const { return ptr_; }
    T* operator->() const { return ptr_; }

    // 释放指针.
    // 返回对象当前拥有的指针. 如果指针为NULL, 返回NULL.
    // 操作完成后, 对象拥有一个NULL指针, 不再拥有任何对象.
    T* release()
    {
        T* retVal = ptr_;
        ptr_ = NULL;
        return retVal;
    }

    scoped_refptr<T>& operator=(T* p)
    {
        // 先调用AddRef, 这样自我赋值也能工作.
        if(p)
        {
            p->AddRef();
        }
        if(ptr_ )
        {
            ptr_ ->Release();
        }
        ptr_ = p;
        return *this;
    }

    scoped_refptr<T>& operator=(const scoped_refptr<T>& r)
    {
        return *this = r.ptr_;
    }

    template<typename U>
    scoped_refptr<T>& operator=(const scoped_refptr<U>& r)
    {
        return *this = r.get();
    }

    void swap(T** pp)
    {
        T* p = ptr_;
        ptr_ = *pp;
        *pp = p;
    }

    void swap(scoped_refptr<T>& r)
    {
        swap(&r.ptr_);
    }

protected:
    T* ptr_;
};

template<typename T>
scoped_refptr<T> make_scoped_refptr(T* t)
{
    return scoped_refptr<T>(t);
}

#endif //__base_ref_counted_h__