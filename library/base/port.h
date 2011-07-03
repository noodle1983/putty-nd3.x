
#ifndef __base_port_h__
#define __base_port_h__

#pragma once

#include "build_config.h"

#define GG_LONGLONG(x)      x##I64
#define GG_ULONGLONG(x)     x##UI64

#define GG_INT8_C(x)        (x)
#define GG_INT16_C(x)       (x)
#define GG_INT32_C(x)       (x)
#define GG_INT64_C(x)       GG_LONGLONG(x)

#define GG_UINT8_C(x)       (x ## U)
#define GG_UINT16_C(x)      (x ## U)
#define GG_UINT32_C(x)      (x ## U)
#define GG_UINT64_C(x)      GG_ULONGLONG(x)

#define GG_VA_COPY(a, b)    (a = b)

#define API_CALL            __stdcall

#endif //__base_port_h__