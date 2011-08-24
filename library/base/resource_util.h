
#ifndef __base_resource_util_h__
#define __base_resource_util_h__

#pragma once

#include <windows.h>

namespace base
{

    // 从dll中获取数据资源.
    bool GetDataResourceFromModule(HMODULE module, int resource_id,
        void** data, size_t* length);

} //namespace base

#endif //__base_resource_util_h__