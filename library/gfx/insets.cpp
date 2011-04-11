
#include "insets.h"

#include "base/stringprintf.h"

namespace gfx
{

    std::string Insets::ToString() const
    {
        // 按照构造函数参数的顺序打印出成员变量.
        return base::StringPrintf("%d,%d,%d,%d",
            top_, left_, bottom_, right_);
    }

} //namespace gfx