
#ifndef __base_auto_reset_h__
#define __base_auto_reset_h__

#pragma once

#include "basic_types.h"

// AutoResetValue用于设定变量在特定作用域范围内的值. 举个例子, 如果你的
// 代码块出口点有"var = false;"或者"var = old_var;", 可以考虑使用这个类.
//
// 尽管很简单, 但需要注意AutoResetValue实例的生命周期比scoped_variable短,
// 以免AutoResetValue离开作用域时写非法内存.
template<typename T>
class AutoReset
{
public:
    AutoReset(T* scoped_variable, T new_value)
        : scoped_variable_(scoped_variable),
        original_value_(*scoped_variable)
    {
        *scoped_variable_ = new_value;
    }

    ~AutoReset() { *scoped_variable_ = original_value_; }

private:
    T* scoped_variable_;
    T original_value_;

    DISALLOW_COPY_AND_ASSIGN(AutoReset);
};

#endif //__base_auto_reset_h__