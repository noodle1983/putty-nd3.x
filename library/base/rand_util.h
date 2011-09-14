
#ifndef __base_rand_util_h__
#define __base_rand_util_h__

#pragma once

#include <string>

#include "basic_types.h"

namespace base
{

    // 返回[0, kuint64max]区间的随机数. 线程安全.
    uint64 RandUint64();

    // 返回[min, max]区间的随机数. 线程安全.
    int RandInt(int min, int max);

    // 返回[0, range)区间的随机数. 线程安全.
    //
    // 可用作std::random_shuffle()的适配器:
    // 给定|std::vector<int> myvector|, 像这样打乱
    //     std::random_shuffle(myvector.begin(), myvector.end(), base::RandGenerator);
    uint64 RandGenerator(uint64 range);

    // 返回[0, 1)区间的随机数. 线程安全.
    double RandDouble();

    // Given input |bits|, convert with maximum precision to a double in
    // the range [0, 1). Thread-safe.
    double BitsToOpenEndedUnitInterval(uint64 bits);

    // Fills |output_length| bytes of |output| with cryptographically strong random
    // data.
    void RandBytes(void* output, size_t output_length);

    // Fills a string of length |length| with with cryptographically strong random
    // data and returns it.
    //
    // Not that this is a variation of |RandBytes| with a different return type.
    std::string RandBytesAsString(size_t length);

    // Generate128BitRandomBase64String returns a string of length 24 containing
    // cryptographically strong random data encoded in base64.
    std::string Generate128BitRandomBase64String();

} //namespace base


#endif //__base_rand_util_h__