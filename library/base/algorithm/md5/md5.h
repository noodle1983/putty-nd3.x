
#ifndef __base_algorithm_md5_h__
#define __base_algorithm_md5_h__

#pragma once

#include <string>

// MD5是信息-摘要算法5(Message Digest algorithm 5).
// MD5是计算机安全领域广泛使用的一种散列函数, 但常用作文件校验.
// 编码复杂且慢, 但是冲突的概率很小.
// 参见:
//   http://en.wikipedia.org/wiki/MD5

// 下列函数用于MD5运算. 最简单的是直接调用MD5Sum()对指定数据生成MD5检验和.
//
// 也可以通过多次调用MD5Update()来计算MD5检验和:
//     MD5Context ctx; // intermediate MD5 data: do not use
//     MD5Init(&ctx);
//     MD5Update(&ctx, data1, length1);
//     MD5Update(&ctx, data2, length2);
//     ...
//
//     MD5Digest digest; // the result of the computation
//     MD5Final(&digest, &ctx);
//
// 调用MD5DigestToBase16()生成摘要串.

// MD5运算输出.
typedef struct MD5Digest_struct
{
    unsigned char a[16];
} MD5Digest;

// 存储MD5运算的中间数据. 调用者不需要直接访问该数据.
typedef char MD5Context[88];

// 对于给定大小的缓冲区计算MD5值. 结构体'digest'用来存储结果.
void MD5Sum(const void* data, size_t length, MD5Digest* digest);

// 为后续初MD5Update()的调用始化MD5 context结构.
void MD5Init(MD5Context* context);

// 使用给定的缓冲区数据更新MD5 context中的校验和, 计算过程中可以多次调用,
// 但是必须在MD5Init()之后调用.
void MD5Update(MD5Context* context, const void* buf, size_t len);

// 完成MD5运算操作并填充digest缓冲区.
void MD5Final(MD5Digest* digest, MD5Context* pCtx);

// 将摘要转化成适合人阅读的16进制.
std::string MD5DigestToBase16(const MD5Digest& digest);

// 返回字符串string的MD5值(16进制串).
std::string MD5String(const std::string& str);

#endif //__base_algorithm_md5_h__