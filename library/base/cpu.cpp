
#include "cpu.h"

#include <intrin.h>

namespace base
{

    CPU::CPU()
        : type_(0),
        family_(0),
        model_(0),
        stepping_(0),
        ext_model_(0),
        ext_family_(0),
        has_mmx_(false),
        has_sse_(false),
        has_sse2_(false),
        has_sse3_(false),
        has_ssse3_(false),
        has_sse41_(false),
        has_sse42_(false),
        cpu_vendor_("unknown")
    {
        Initialize();
    }

    void CPU::Initialize()
    {
#if defined(ARCH_CPU_X86_FAMILY)
        int cpu_info[4] = { -1 };
        char cpu_string[0x20];

        // InfoType参数传0调用__cpuid, CPUInfo[0]中返回最大合法的Id值,
        // CPU标识字符串在另外三个数组元素中, 不是顺序存放. 下面的代码
        // 对信息进行可读性排序.
        //
        // 更多信息参见:
        // http://msdn.microsoft.com/en-us/library/hskdteyh.aspx
        __cpuid(cpu_info, 0);
        int num_ids = cpu_info[0];
        memset(cpu_string, 0, sizeof(cpu_string));
        *(reinterpret_cast<int*>(cpu_string)) = cpu_info[1];
        *(reinterpret_cast<int*>(cpu_string+4)) = cpu_info[3];
        *(reinterpret_cast<int*>(cpu_string+8)) = cpu_info[2];

        // 解释CPU特征信息.
        if(num_ids > 0)
        {
            __cpuid(cpu_info, 1);
            stepping_ = cpu_info[0] & 0xf;
            model_ = ((cpu_info[0] >> 4) & 0xf) + ((cpu_info[0] >> 12) & 0xf0);
            family_ = (cpu_info[0] >> 8) & 0xf;
            type_ = (cpu_info[0] >> 12) & 0x3;
            ext_model_ = (cpu_info[0] >> 16) & 0xf;
            ext_family_ = (cpu_info[0] >> 20) & 0xff;
            cpu_vendor_ = cpu_string;
            has_mmx_ = (cpu_info[3] & 0x00800000) != 0;
            has_sse_ = (cpu_info[3] & 0x02000000) != 0;
            has_sse2_ = (cpu_info[3] & 0x04000000) != 0;
            has_sse3_ = (cpu_info[2] & 0x00000001) != 0;
            has_ssse3_ = (cpu_info[2] & 0x00000200) != 0;
            has_sse41_ = (cpu_info[2] & 0x00080000) != 0;
            has_sse42_ = (cpu_info[2] & 0x00100000) != 0;
        }
#endif
    }

} //namespace base