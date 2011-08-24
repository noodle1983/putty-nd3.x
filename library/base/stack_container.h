
#ifndef __base_stack_container_h__
#define __base_stack_container_h__

#pragma once

#include <string>
#include <vector>

#include "basic_types.h"

// 能为STL容器提供栈缓冲区分配内存的分配器, 溢出时在堆上分配. 栈缓冲区分
// 配内存可以在一定程度上避免堆分配.
//
// STL喜欢拷贝分配器, 因此分配器本身不能保存实际数据. 我们让容器负责创建
// StackAllocator::Source存储数据. 拷贝分配器只是共享一个source指针, 所有
// 基于StackAllocator创建的分配器都共享一个栈缓冲区.
//
// 栈缓冲区的实现很简单. 第一次分配时如果栈空间满足则使用栈缓冲区, 后面
// 的任何分配都不再使用栈缓冲区, 即使有剩余空间. 这适合用于类数组的容器,
// 但调用者需要reserve()容器大小到栈缓冲区大小, 否则容器分配一个很小的数
// 组就能"用光"栈缓冲区.
template<typename T, size_t stack_capacity>
class StackAllocator : public std::allocator<T>
{
public:
    typedef typename std::allocator<T>::pointer pointer;
    typedef typename std::allocator<T>::size_type size_type;

    // 分配器支持存储. 容器负责维护这个对象.
    struct Source
    {
        Source() : used_stack_buffer_(false) {}

        // 强转到正确类型.
        T* stack_buffer() { return reinterpret_cast<T*>(stack_buffer_); }
        const T* stack_buffer() const
        {
            return reinterpret_cast<const T*>(stack_buffer_);
        }

        // 重要: 务必保证stack_buffer_是对齐的, 因为它模拟一个类型为T
        // 的数组. 在stack_buffer_前面声明任何非对齐类型(像bool)都需要小心.

        // 缓冲区. 不能是类型T, 因为我们不想构造函数和析构函数被自动调用.
        // 定义一个大小相等的POD缓冲区替换.
        char stack_buffer_[sizeof(T[stack_capacity])];

        // 栈缓冲区被分配使用时设置. 不记录缓冲区使用了多少, 只记录是否有
        // 人在使用.
        bool used_stack_buffer_;
    };

    // 当容器想要引用一个类型为U的分配器时使用.
    template<typename U>
    struct rebind
    {
        typedef StackAllocator<U, stack_capacity> other;
    };

    // 直接使用拷贝构造函数, 可以共享存储.
    StackAllocator(const StackAllocator<T, stack_capacity>& rhs)
        : std::allocator<T>(), source_(rhs.source_) {}

    // ISO C++要求定义下面的构造函数, 如果不定义, VC++2008SP1 Release
    // 中的std::vector会在类_Container_base_aux_alloc_real(来自
    // <xutility>)报错.
    // 对于这个构造函数, 我们不能共享存储, 因为无法保证Source的空间一
    // 定比U的大.
    // TODO: 如果非要苛求, 当sizeof(T)==sizeof(U)时允许共享存储.
    template<typename U, size_t other_capacity>
    StackAllocator(const StackAllocator<U, other_capacity>& other)
        : source_(NULL) {}

    explicit StackAllocator(Source* source) : source_(source) {}

    // 实际分配工作. 使用栈缓冲区, 如果空间还没被使用且大小足够. 否则交由标准
    // 标准分配器处理.
    pointer allocate(size_type n, void* hint=0)
    {
        if(source_!=NULL && !source_->used_stack_buffer_ && n<=stack_capacity)
        {
            source_->used_stack_buffer_ = true;
            return source_->stack_buffer();
        }
        else
        {
            return std::allocator<T>::allocate(n, hint);
        }
    }

    // 释放: 尝试释放栈缓冲区时, 只用标记上没用即可. 对于非栈缓冲区指针, 交由
    // 标准分配器处理.
    void deallocate(pointer p, size_type n)
    {
        if(source_!=NULL && p==source_->stack_buffer())
        {
            source_->used_stack_buffer_ = false;
        }
        else
        {
            std::allocator<T>::deallocate(p, n);
        }
    }

private:
    Source* source_;
};

// 基于STL容器的封装, 维护一个栈缓冲区, 大小等于容器的初始容量. 容器增长超过栈
// 缓冲区大小时会自动在堆上分配内存. 容器必须支持reserve().
//
// 小心: ContainerType必须使用正确的StackAllocator类型. 这个类主要是内部使用.
// 请使用下面的封装类.
template<typename TContainerType, int stack_capacity>
class StackContainer
{
public:
    typedef TContainerType ContainerType;
    typedef typename ContainerType::value_type ContainedType;
    typedef StackAllocator<ContainedType, stack_capacity> Allocator;

    // Allocator必须先于container构造!
    StackContainer() : allocator_(&stack_data_), container_(allocator_)
    {
        // 在所有操作之前, 容器通过预留stack_capacity使用栈分配.
        container_.reserve(stack_capacity);
    }

    // 获取实际的容器.
    //
    // 危险: 使用返回值拷贝构造出来的对象生命周期必须比源对象短. 拷贝对象会
    // 共享同一分配器, 所以栈缓冲区都是一份. 如果需要生命周期更长的对象, 使
    // 用std::copy拷贝到真的容器中.
    ContainerType& container() { return container_; }
    const ContainerType& container() const { return container_; }

    // 支持->操作获取容器:
    //     StackContainer<...> foo;
    //     std::sort(foo->begin(), foo->end());
    ContainerType* operator->() { return &container_; }
    const ContainerType* operator->() const { return &container_; }

protected:
    typename Allocator::Source stack_data_;
    Allocator allocator_;
    ContainerType container_;

    DISALLOW_COPY_AND_ASSIGN(StackContainer);
};

// StackString
template<size_t stack_capacity>
class StackString : public StackContainer<
    std::basic_string<char,
    std::char_traits<char>,
    StackAllocator<char, stack_capacity> >,
    stack_capacity>
{
public:
    StackString() : StackContainer<
        std::basic_string<char,
        std::char_traits<char>,
        StackAllocator<char, stack_capacity> >,
        stack_capacity>() {}

private:
    DISALLOW_COPY_AND_ASSIGN(StackString);
};

// StackWString
template<size_t stack_capacity>
class StackWString : public StackContainer<
    std::basic_string<wchar_t,
    std::char_traits<wchar_t>,
    StackAllocator<wchar_t, stack_capacity> >,
    stack_capacity>
{
public:
    StackWString() : StackContainer<
        std::basic_string<wchar_t,
        std::char_traits<wchar_t>,
        StackAllocator<wchar_t, stack_capacity> >,
        stack_capacity>() {}

private:
    DISALLOW_COPY_AND_ASSIGN(StackWString);
};

// StackVector
//
// 示例:
//     StackVector<int, 16> foo;
//     foo->push_back(22);  // 重载了->操作符.
//     foo[0] = 10;         // 重载了[]操作符.
template<typename T, size_t stack_capacity>
class StackVector : public StackContainer<
    std::vector<T, StackAllocator<T, stack_capacity> >,
    stack_capacity>
{
public:
    StackVector() : StackContainer<
        std::vector<T, StackAllocator<T, stack_capacity> >,
        stack_capacity>() {}

    // 有时会用在一些需要元素有拷贝构造函数的STL容器中. 不能调用常规的拷贝构造
    // 函数, 因为那样会取走源对象的栈缓冲区. 这里, 创建一个空对象, 构造自己的
    // 栈缓冲区.
    StackVector(const StackVector<T, stack_capacity>& other)
        : StackContainer<
        std::vector<T, StackAllocator<T, stack_capacity> >,
        stack_capacity>()
    {
        this->container().assign(other->begin(), other->end());
    }

    StackVector<T, stack_capacity>& operator=(
        const StackVector<T, stack_capacity>& other)
    {
        this->container().assign(other->begin(), other->end());
        return *this;
    }

    T& operator[](size_t i) { return this->container().operator[](i); }
    const T& operator[](size_t i) const
    {
        return this->container().operator[](i);
    }
};

#endif //__base_stack_container_h__