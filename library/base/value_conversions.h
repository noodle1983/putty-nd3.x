
#ifndef __base_value_conversionss_h__
#define __base_value_conversionss_h__

#pragma once

// |FilePath|和|Value|对象互相转换方法.

class FilePath;
class StringValue;
class Value;

namespace base
{

    // 调用者接管返回值的生命周期.
    StringValue* CreateFilePathValue(const FilePath& in_value);
    bool GetValueAsFilePath(const Value& value, FilePath* file_path);

}

#endif //__base_value_conversionss_h__