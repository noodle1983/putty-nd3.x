
// 用于跟踪引用的"智能"指针类型. 每个指向特定对象的指针维护在一个首位链接的
// 列表中. 当指向对象的最后一个指针销毁或者重新赋值时, 对象被销毁.
// 使用时要小心, 对象最后的引用解除时会被销毁.
// 几点警告:
// - 和所有引用计数模式一样, 循环引用会导致泄漏.
// - 每个智能指针实际上是2个指针(8字节而不是4字节).
// - 每次指针释放时, 会遍历整个链表. 因此该类不适合指向特定对象指针多余2-3个
//   的情况.
// - 引用只会在linked_ptr<>对象拷贝的时候维护. linked_ptr<>和原始指针转换时
//   会发生不好的情况(两次删除).
//
// 该类的一个应用场景是对于STL容器中的对象引用进行排序. linked_ptr<>可以安全的
// 应用于vector<>中. 其它地方不是太适合使用.
//
// 注意: 如果在linked_ptr<>使用不完整的类型, 包含linked_ptr<>的类必须有构造和
// 析构函数(即使它们什么都不做!).
//
// 线程安全:
//   linked_ptr是非线程安全的. 拷贝linked_ptr对象实际上是一次读写操作.
//
// 其它: 对于shared_ptr中的linked_ptr,
//  - 大小为2个指针(对于32位地址是8字节)
//  - 拷贝和删除是线程安全的
//  - 支持weak_ptrs

#ifndef __base_linked_ptr_h__
#define __base_linked_ptr_h__

#pragma once

#include "base/logging.h"

// linked_ptr_internal在所有linked_ptr<>实例中使用. 需要一个非模板类是因为
// 不同类型的linked_ptr<>会引用相同对象(linked_ptr<Superclass>(obj) vs
// linked_ptr<Subclass>(obj)). 所以, 不同类型的linked_ptr可能会出现在相同的
// 链表中, 因此这里需要一个单独的类.
//
// 请不要直接使用这个类. 使用linked_ptr<T>.
class linked_ptr_internal
{
public:
    // 创建一个新的环只含有本实例.
    void join_new()
    {
        next_ = this;
    }

    // 加入一个存在的环.
    void join(linked_ptr_internal const* ptr)
    {
        next_ = ptr->next_;
        ptr->next_ = this;
    }

    // 离开加入的环. 如果是环的最后一个成员返回true. 一旦调用成功, 可以再次
    // join()到其它环.
    bool depart()
    {
        if(next_ == this) return true;
        linked_ptr_internal const* p = next_;
        while(p->next_ != this) p = p->next_;
        p->next_ = next_;
        return false;
    }

private:
    mutable linked_ptr_internal const* next_;
};

template<typename T>
class linked_ptr
{
public:
    typedef T element_type;

    // 接管指针的所有权. 对象创建后尽快调用.
    explicit linked_ptr(T* ptr = NULL) { capture(ptr); }
    ~linked_ptr() { depart(); }

    // 拷贝存在的linked_ptr<>, 添加到引用队列.
    template<typename U> linked_ptr(linked_ptr<U> const& ptr) { copy(&ptr); }

    linked_ptr(linked_ptr const& ptr)
    {
        DCHECK_NE(&ptr, this);
        copy(&ptr);
    }

    // 赋值操作会释放旧值接受新值.
    template<typename U> linked_ptr& operator=(linked_ptr<U> const& ptr)
    {
        depart();
        copy(&ptr);
        return *this;
    }

    linked_ptr& operator=(linked_ptr const& ptr)
    {
        if(&ptr != this)
        {
            depart();
            copy(&ptr);
        }
        return *this;
    }

    // 只能指针成员.
    void reset(T* ptr = NULL)
    {
        depart();
        capture(ptr);
    }
    T* get() const { return value_; }
    T* operator->() const { return value_; }
    T& operator*() const { return *value_; }
    // 释放指针对象的所有权并返回. 需要linked_ptr对对象拥有独立的所有权.
    T* release()
    {
        bool last = link_.depart();
        CHECK(last);
        T* v = value_;
        value_ = NULL;
        return v;
    }

    bool operator==(const T* p) const { return value_ == p; }
    bool operator!=(const T* p) const { return value_ != p; }
    template<typename U>
    bool operator==(linked_ptr<U> const& ptr) const
    {
        return value_ == ptr.get();
    }
    template<typename U>
    bool operator!=(linked_ptr<U> const& ptr) const
    {
        return value_ != ptr.get();
    }

private:
    template<typename U>
    friend class linked_ptr;

    T* value_;
    linked_ptr_internal link_;

    void depart()
    {
        if (link_.depart()) delete value_;
    }

    void capture(T* ptr)
    {
        value_ = ptr;
        link_.join_new();
    }

    template<typename U> void copy(linked_ptr<U> const* ptr)
    {
        value_ = ptr->get();
        if(value_)
        {
            link_.join(&ptr->link_);
        }
        else
        {
            link_.join_new();
        }
    }
};

template<typename T> inline
bool operator==(T* ptr, const linked_ptr<T>& x)
{
    return ptr == x.get();
}

template<typename T> inline
bool operator!=(T* ptr, const linked_ptr<T>& x)
{
    return ptr != x.get();
}

// 函数转换T*为linked_ptr<T>.
// make_linked_ptr(new FooBarBaz<type>(arg))是
// linked_ptr<FooBarBaz<type> >(new FooBarBaz<type>(arg))的简化形式.
template<typename T>
linked_ptr<T> make_linked_ptr(T* ptr)
{
    return linked_ptr<T>(ptr);
}

#endif //__base_linked_ptr_h__