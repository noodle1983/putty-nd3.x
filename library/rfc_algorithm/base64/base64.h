
#ifndef __rfc_algorithm_base64_h__
#define __rfc_algorithm_base64_h__

#pragma once

#include <string>

namespace base
{

    // base64编码. 成功返回true失败返回false. output只在成功时被修改.
    bool Base64Encode(const std::string& input, std::string* output);

    // base64解码. 成功返回true失败返回false. output只在成功时被修改.
    bool Base64Decode(const std::string& input, std::string* output);

} //namespace base

#endif //__rfc_algorithm_base64_h__