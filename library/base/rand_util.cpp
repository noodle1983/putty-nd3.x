
#include "rand_util.h"

#include <limits>
#include <stdlib.h>

#include "logging.h"

namespace base
{

    uint32 RandUint32()
    {
        uint32 number;
        CHECK_EQ(rand_s(&number), 0);
        return number;
    }

    uint64 RandUint64()
    {
        uint32 first_half = RandUint32();
        uint32 second_half = RandUint32();
        return (static_cast<uint64>(first_half)<<32)+second_half;
    }

    int RandInt(int min, int max)
    {
        DCHECK(min <= max);

        uint64 range = static_cast<int64>(max) - min + 1;
        uint64 number = RandUint64();
        int result = min + static_cast<int>(number%range);
        DCHECK(result>=min && result<=max);
        return result;
    }

    double RandDouble()
    {
        // 为了提高精度, 生成位数尽量多的数字, 然后进行适当的幂运算得到[0, 1)区间
        // 的double数值. IEEE 754精度要求是53位.
        COMPILE_ASSERT(std::numeric_limits<double>::radix==2, otherwise_use_scalbn);
        static const int kBits = std::numeric_limits<double>::digits;
        uint64 random_bits = RandUint64() & ((1UI64<<kBits)-1);
        double result = ldexp(static_cast<double>(random_bits), -1*kBits);
        DCHECK(result>=0.0 && result<1.0);
        return result;
    }

    uint64 RandGenerator(uint64 max)
    {
        DCHECK(max > 0);
        return RandUint64() % max;
    }

} //namespace base