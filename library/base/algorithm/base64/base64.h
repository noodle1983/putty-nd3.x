
#ifndef __base_base64_h__
#define __base_base64_h__

#pragma once

#include "base/string_piece.h"

namespace base
{

    // base64编码. 成功返回true失败返回false. output只在成功时被修改.
    bool Base64Encode(const StringPiece& input, std::string* output);

    // base64解码. 成功返回true失败返回false. output只在成功时被修改.
    bool Base64Decode(const StringPiece& input, std::string* output);

} //namespace base

#endif //__base_base64_h__