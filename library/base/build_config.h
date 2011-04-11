
#ifndef __base_build_config_h__
#define __base_build_config_h__

#pragma once

#define OS_WIN              1

#define COMPILER_MSVC       1

#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86_64     1
#define ARCH_CPU_64_BITS    1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86        1
#define ARCH_CPU_32_BITS    1
#endif

#endif //__base_build_config_h__