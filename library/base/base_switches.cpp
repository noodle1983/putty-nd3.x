
#include "base_switches.h"

namespace base
{

    // 指定缺省的最大激活日志记录等级, 默认是0, 一般使用正数.
    const char kV[]                             = "v";

    // 给出每个模块的最大激活日志记录等级, 覆盖--v提供的值. 例如:
    // "my_module=2,foo*=3"将改变所有"my_module.*"和"foo*.*"(
    // 匹配时"-inl"后缀会被预先忽略掉)源文件中代码的日志等级.
    //
    // 含有\或者/的模式会匹配整个路径而不是一个模块. 例如
    // "*/foo/bar/*=2"会改变"foo/bar"目录下所有源文件中代码
    // 的日志等级.
    const char kVModule[]                       = "vmodule";

} //namespace base