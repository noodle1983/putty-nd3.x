
#ifndef __base__ref_counted_memory_h__
#define __base__ref_counted_memory_h__

#pragma once

#include <string>
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
        : data_(length ? data : NULL), length_(length) {}

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

    const std::vector<unsigned char>& data() const { return data_; }
    std::vector<unsigned char>& data() { return data_; }

private:
    friend class base::RefCountedThreadSafe<RefCountedBytes>;
    virtual ~RefCountedBytes();

    std::vector<unsigned char> data_;

    DISALLOW_COPY_AND_ASSIGN(RefCountedBytes);
};

namespace base
{

    // An implementation of RefCountedMemory, where the bytes are stored in an STL
    // string. Use this if your data naturally arrives in that format.
    class RefCountedString : public RefCountedMemory
    {
    public:
        RefCountedString();

        // Constructs a RefCountedString object by performing a swap. (To non
        // destructively build a RefCountedString, use the default constructor and
        // copy into object->data()).
        static RefCountedString* TakeString(std::string* to_destroy);

        // Overridden from RefCountedMemory:
        virtual const unsigned char* front() const;
        virtual size_t size() const;

        const std::string& data() const { return data_; }
        std::string& data() { return data_; }

    private:
        friend class base::RefCountedThreadSafe<RefCountedString>;
        virtual ~RefCountedString();

        std::string data_;

        DISALLOW_COPY_AND_ASSIGN(RefCountedString);
    };

} //namespace base

#endif //__base__ref_counted_memory_h__