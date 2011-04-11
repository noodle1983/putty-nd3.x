
#ifndef __base_pickle_h__
#define __base_pickle_h__

#pragma once

#include <string>

#include "logging.h"
#include "string16.h"

// 为二进制数据封包解包提供基础设施.
//
// Pickle类支持添加基础数据(ints、strings等), 动态增加内存以存储数据序列.
// 通过"data"暴露Pickle内部存储, 可用于初始化Pickle对象以读取数据.
//
// 从Pickle对象读取数据时, 使用者需要知道读取的数据类型和次序, 因为Pickle
// 写数据的时候没有维护类型信息, 这一点很重要.
//
// Pickle的数据前面有一个头, 记录了数据的有效负载, 也可支持其它信息存储.
// 头占用的大小通过Pickle构造函数的header_size参数指定.

class Pickle
{
public:
    virtual ~Pickle();

    // 使用缺省的头长度初始化Pickle对象.
    Pickle();

    // 使用指定大小的头长度(字节)初始化Pickle对象, 必须大于等于
    // sizeof(Pickle::Header). header_size会向上取整保证32位对齐.
    explicit Pickle(int header_size);

    // 使用一块内存数据初始化Pickle对象, Pickle只是引用而不拷贝数据. 这种方式
    // 初始化的Pickle只允许调用const方法. 头占用大小可以通过data_len推导出来.
    Pickle(const char* data, int data_len);

    // 深拷贝构造.
    Pickle(const Pickle& other);

    // 深赋值构造.
    Pickle& operator=(const Pickle& other);

    // 返回Pickle的数据长度.
    int size() const
    {
        return static_cast<int>(header_size_+header_->payload_size);
    }

    // 返回Pickle的数据.
    const void* data() const { return header_; }

    // 读取Pickle有效数据的方法. 初始化*iter为NULL表示从头开始读. 如果成功, 函数
    // 返回true, 返回false表示无法读取数据到result.
    bool ReadBool(void** iter, bool* result) const;
    bool ReadInt(void** iter, int* result) const;
    bool ReadLong(void** iter, long* result) const;
    bool ReadSize(void** iter, size_t* result) const;
    bool ReadUInt32(void** iter, uint32* result) const;
    bool ReadInt64(void** iter, int64* result) const;
    bool ReadUInt64(void** iter, uint64* result) const;
    bool ReadString(void** iter, std::string* result) const;
    bool ReadWString(void** iter, std::wstring* result) const;
    bool ReadString16(void** iter, string16* result) const;
    bool ReadData(void** iter, const char** data, int* length) const;
    bool ReadBytes(void** iter, const char** data, int length) const;

    // 比ReadInt()安全, 会检查结果是否为负数, 用于读取对象长度.
    bool ReadLength(void** iter, int* result) const;

    // Pickle添加有效数据的方法, 从后端加入. 读取的时候必须和添加顺序一致.
    bool WriteBool(bool value)
    {
        return WriteInt(value ? 1 : 0);
    }
    bool WriteInt(int value)
    {
        return WriteBytes(&value, sizeof(value));
    }
    bool WriteLong(long value)
    {
        return WriteBytes(&value, sizeof(value));
    }
    bool WriteSize(size_t value)
    {
        return WriteBytes(&value, sizeof(value));
    }
    bool WriteUInt32(uint32 value)
    {
        return WriteBytes(&value, sizeof(value));
    }
    bool WriteInt64(int64 value)
    {
        return WriteBytes(&value, sizeof(value));
    }
    bool WriteUInt64(uint64 value)
    {
        return WriteBytes(&value, sizeof(value));
    }
    bool WriteString(const std::string& value);
    bool WriteWString(const std::wstring& value);
    bool WriteString16(const string16& value);
    bool WriteData(const char* data, int length);
    bool WriteBytes(const void* data, int data_len);

    // 类似WriteData, 但允许调用者直接写Pickle对象. 当待写数据不在内存中时
    // 可以节省一次拷贝操作.调用者需要注意写数据长度不要超过请求的. 读取
    // 数据用ReadData. 失败返回NULL.
    //
    // 返回的指针在下次写操作前是有效的.
    char* BeginWriteData(int length);

    // 对于可变长度的缓冲区(比如通过BeginWriteData创建的), 如果数据需要的长度
    // 小于原始请求的, 会进行削减. 举例来说, 你创建了一块10K的数据缓冲区, 但
    // 实际只填充了10字节数据. 使用这个函数可以把剩余的9990字节削减掉. 缓冲区
    // 大小不能增加, 只能减少. 函数假定缓冲区长度从未改变过.
    void TrimWriteData(int length);

    // 有效数据跟在Header后面(头长度可定制).
    struct Header
    {
        uint32 payload_size; // 有效数据长度.
    };

    // 返回头信息, 强转到用户指定类型. T类型必须是Header的子类, 大小和传递
    // 给Pickle构造函数的header_size参数一致.
    template<class T>
    T* headerT()
    {
        DCHECK(sizeof(T) == header_size_);
        return static_cast<T*>(header_);
    }
    template<class T>
    const T* headerT() const
    {
        DCHECK(sizeof(T) == header_size_);
        return static_cast<const T*>(header_);
    }

    // 如果迭代器指向的数据长度>=len返回true. 如果空间不足返回false.
    bool IteratorHasRoomFor(const void* iter, int len) const
    {
        if((len<0) || (iter<header_) || iter>end_of_payload())
        {
            return false;
        }
        const char* end_of_region = reinterpret_cast<const char*>(iter) + len;
        // 当心封装函数的指针运算溢出.
        return (iter<=end_of_region) && (end_of_region<=end_of_payload());
    }

protected:
    size_t payload_size() const { return header_->payload_size; }

    char* payload()
    {
        return reinterpret_cast<char*>(header_) + header_size_;
    }
    const char* payload() const
    {
        return reinterpret_cast<const char*>(header_) + header_size_;
    }

    // 返回有效数据后面的内存地址.
    char* end_of_payload()
    {
        return payload() + payload_size();
    }
    const char* end_of_payload() const
    {
        return payload() + payload_size();
    }

    size_t capacity() const
    {
        return capacity_;
    }

    // 为写数据准备缓冲区, 返回的指针是写地址, 发生错误时返回NULL.
    // 调用EndWrite为下一次写操作对齐数据.
    char* BeginWrite(size_t length);

    // 用NULL字节补白使数据对齐, 完成写操作. 应该与BeginWrite配对使用, 数据写入
    // 后不必要再调用.
    void EndWrite(char* dest, int length);

    // 调整容量, 注意new_capacity大小应该包括头大小:
    // new_capacity = sizeof(Header) + desired_payload_capacity.
    // realloc()失败将会导致函数失败, 调用者需要检查返回值是否为true.
    bool Resize(size_t new_capacity);

    // 'i'向上取整为'alignment'的倍数.
    static size_t AlignInt(size_t i, int alignment)
    {
        return i + (alignment - (i % alignment)) % alignment;
    }

    // 迭代器向前移动一定数量的字节, 确保对齐. 传入的指针不是对齐的, 但修改后
    // 一定是sizeof(uint32)的倍数.
    static void UpdateIter(void** iter, int bytes)
    {
        *iter = static_cast<char*>(*iter) + AlignInt(bytes, sizeof(uint32));
    }

    // 有效数据分配粒度.
    static const int kPayloadUnit;

private:
    Header* header_;
    size_t header_size_; // 支持其它信息存储.
    // 有效数据内存分配的大小(-1表示只读).
    size_t capacity_;
    size_t variable_buffer_offset_; // 如果非0, 表示缓冲区偏移.
};

#endif //__base_pickle_h__