
#include "ref_counted_memory.h"

RefCountedMemory::RefCountedMemory() {}

RefCountedMemory::~RefCountedMemory() {}

const unsigned char* RefCountedStaticMemory::front() const
{
    return data_;
}

size_t RefCountedStaticMemory::size() const
{
    return length_;
}


RefCountedBytes::RefCountedBytes() {}

RefCountedBytes::RefCountedBytes(const std::vector<unsigned char>& initializer)
: data(initializer) {}

RefCountedBytes* RefCountedBytes::TakeVector(
    std::vector<unsigned char>* to_destroy)
{
    RefCountedBytes* bytes = new RefCountedBytes;
    bytes->data.swap(*to_destroy);
    return bytes;
}

const unsigned char* RefCountedBytes::front() const
{
    // 如果对一个空的vector调用front(), STL会断言, 但是调用者希望返回NULL.
    return size() ? &data.front() : NULL;
}

size_t RefCountedBytes::size() const
{
    return data.size();
}

RefCountedBytes::~RefCountedBytes() {}