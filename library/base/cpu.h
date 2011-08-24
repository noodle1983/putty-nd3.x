
#ifndef __base_cpu_h__
#define __base_cpu_h__

#pragma once

#include <string>

namespace base
{

    // 查询处理器(CPU)信息.
    class CPU
    {
    public:
        CPU();

        // CPU信息访问接口
        const std::string& vendor_name() const { return cpu_vendor_; }
        int stepping() const { return stepping_; }
        int model() const { return model_; }
        int family() const { return family_; }
        int type() const { return type_; }
        int extended_model() const { return ext_model_; }
        int extended_family() const { return ext_family_; }
        int has_mmx() const { return has_mmx_; }
        int has_sse() const { return has_sse_; }
        int has_sse2() const { return has_sse2_; }
        int has_sse3() const { return has_sse3_; }
        int has_ssse3() const { return has_ssse3_; }
        int has_sse41() const { return has_sse41_; }
        int has_sse42() const { return has_sse42_; }

    private:
        // 查询处理器的CPUID信息.
        void Initialize();

        int type_;     // process type
        int family_;   // family of the processor
        int model_;    // model of processor
        int stepping_; // processor revision number
        int ext_model_;
        int ext_family_;
        bool has_mmx_;
        bool has_sse_;
        bool has_sse2_;
        bool has_sse3_;
        bool has_ssse3_;
        bool has_sse41_;
        bool has_sse42_;
        std::string cpu_vendor_;
    };

} //namespace base

#endif //__base_cpu_h__