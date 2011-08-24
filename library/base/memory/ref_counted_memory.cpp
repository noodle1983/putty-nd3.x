
#include "ref_counted_memory.h"

#include "base/logging.h"

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
: data_(initializer) {}

RefCountedBytes* RefCountedBytes::TakeVector(
    std::vector<unsigned char>* to_destroy)
{
    RefCountedBytes* bytes = new RefCountedBytes;
    bytes->data_.swap(*to_destroy);
    return bytes;
}

const unsigned char* RefCountedBytes::front() const
{
    // 如果对一个空的vector调用front(), STL会断言, 但是调用者希望返回NULL.
    return size() ? &data_.front() : NULL;
}

size_t RefCountedBytes::size() const
{
    return data_.size();
}

RefCountedBytes::~RefCountedBytes() {}

namespace base
{

    RefCountedString::RefCountedString() {}

    RefCountedString::~RefCountedString() {}

    // static
    RefCountedString* RefCountedString::TakeString(std::string* to_destroy)
    {
        RefCountedString* self = new RefCountedString;
        to_destroy->swap(self->data_);
        return self;
    }

    const unsigned char* RefCountedString::front() const
    {
        return data_.empty() ? NULL :
            reinterpret_cast<const unsigned char*>(data_.data());
    }

    size_t RefCountedString::size() const
    {
        return data_.size();
    }

} //namespace base