
#ifndef __view_view_prop_h__
#define __view_view_prop_h__

#pragma once

#include "base/basic_types.h"
#include "base/memory/ref_counted.h"

namespace view
{

    // ViewProp维护视图的键/值对, 用于替换Win32的SetProp, 但是不利用窗口内存管理.
    // ViewProp和SetProp的语义一致, 视图/键来自于最后一次创建的ViewProp.
    class ViewProp
    {
    public:
        // 关联视图/键的值. 如果已经存在, 会替换.
        //
        // ViewProp不会拷贝char*, 指针用于排序.
        ViewProp(HWND view, const char* key, void* data);
        ~ViewProp();

        // 返回视图/键对应的数据, 不存在返回NULL.
        static void* GetValue(HWND view, const char* key);

        // 返回键.
        const char* Key() const;

    private:
        class Data;

        // 存储实际的数据.
        scoped_refptr<Data> data_;

        DISALLOW_COPY_AND_ASSIGN(ViewProp);
    };

} //namespace view

#endif //__view_view_prop_h__