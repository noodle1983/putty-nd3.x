
#ifndef __base__ref_counted_memory_h__
#define __base__ref_counted_memory_h__

#pragma once

#include <vector>

#include "ref_counted.h"


// 内存的通用接口. 类采用了引用计数, 因为它的两个子类中有一个拥有数据,
// 我们需要多态的实现这两种内存容器.
class RefCountedMemory : public base::RefCountedThreadSafe<RefCountedMemory>
{
public:
    // 返回数据的起始地址指针. 如果数据为空, 返回NULL.
    virtual const unsigned char* front() const = 0;

    // 内存大小.
    virtual size_t size() const = 0;

protected:
    friend class base::RefCountedThreadSafe<RefCountedMemory>;
    RefCountedMemory();
    virtual ~RefCountedMemory();
};

// 引用计数无关的RefCountedMemory实现.
class RefCountedStaticMemory : public RefCountedMemory
{
public:
    RefCountedStaticMemory() : data_(NULL), length_(0) {}
    RefCountedStaticMemory(const unsigned char* data, size_t length)
        : data_(data), length_(length) {}

    // 重载自RefCountedMemory:
    virtual const unsigned char* front() const;
    virtual size_t size() const;

private:
    const unsigned char* data_;
    size_t length_;

    DISALLOW_COPY_AND_ASSIGN(RefCountedStaticMemory);
};

// 用vector容纳数据的RefCountedMemory实现.
class RefCountedBytes : public RefCountedMemory
{
public:
    RefCountedBytes();

    // 通过拷贝|initializer|构造一个RefCountedBytes对象.
    RefCountedBytes(const std::vector<unsigned char>& initializer);

    // 通过swap构造一个RefCountedBytes对象.
    static RefCountedBytes* TakeVector(std::vector<unsigned char>* to_destroy);

    // 重载自RefCountedMemory:
    virtual const unsigned char* front() const;
    virtual size_t size() const;

    std::vector<unsigned char> data;

protected:
    friend class base::RefCountedThreadSafe<RefCountedBytes>;
    virtual ~RefCountedBytes();

private:
    DISALLOW_COPY_AND_ASSIGN(RefCountedBytes);
};

#endif //__base__ref_counted_memory_h__