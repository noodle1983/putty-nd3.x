
#ifndef __base_algorithm_sha2_h__
#define __base_algorithm_sha2_h__

#pragma once

#include <string>

namespace base
{

    enum
    {
        SHA256_LENGTH = 32 // SHA-256的哈希值长度.
    };

    // 计算输入字符串|str|的SHA-256哈希并返回哈希值到output缓冲区.
    // 如果'len'>32, 只会存储32字节(哈希总长度)到'output'缓冲区.
    void SHA256HashString(const std::string& str, void* output, size_t len);

    // 32字节字符串易用版本.
    std::string SHA256HashString(const std::string& str);

} //namespace base

#endif //__base_algorithm_sha2_h__