
#ifndef __base_rand_util_h__
#define __base_rand_util_h__

#pragma once

#include "basic_types.h"

namespace base
{

    // 返回[0, kuint64max]区间的随机数. 线程安全.
    uint64 RandUint64();

    // 返回[min, max]区间的随机数. 线程安全.
    int RandInt(int min, int max);

    // 返回[0, max)区间的随机数. 线程安全.
    //
    // 可用作std::random_shuffle()的适配器:
    // 给定|std::vector<int> myvector|, 像这样打乱
    //     std::random_shuffle(myvector.begin(), myvector.end(), base::RandGenerator);
    uint64 RandGenerator(uint64 max);

    // 返回[0, 1)区间的随机数. 线程安全.
    double RandDouble();

} //namespace base


#endif //__base_rand_util_h__