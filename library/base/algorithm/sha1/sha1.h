
#ifndef __base_algorithm_sha1_h__
#define __base_algorithm_sha1_h__

#pragma once

#include <string>

namespace base
{

    enum
    {
        SHA1_LENGTH = 20 // SHA-1的哈希值长度.
    };

    // 计算输入字符串|str|的SHA-1哈希并返回哈希值.
    std::string SHA1HashString(const std::string& str);

    // 计算|data|缓冲区中|len|字节长度的SHA-1哈希存储到|hash|中.
    // |hash|长度必须满足SHA1_LENGTH字节长度.
    void SHA1HashBytes(const unsigned char* data, size_t len, unsigned char* hash);

} //namespace base

#endif //__base_algorithm_sha1_h__