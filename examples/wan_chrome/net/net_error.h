
#ifndef __wan_chrome_net_net_error_h__
#define __wan_chrome_net_net_error_h__

#pragma once

namespace net
{

    // Error domain of the net module's error codes.
    extern const char kErrorDomain[];

    // Error values are negative.
    enum Error
    {
        // No error.
        OK = 0,

#define NET_ERROR(label, value) ERR_ ## label = value,
#include "net_error_list.h"
#undef NET_ERROR

        // The value of the first certificate error code.
        ERR_CERT_BEGIN = ERR_CERT_COMMON_NAME_INVALID,
    };

    // Returns a textual representation of the error code for logging purposes.
    const char* ErrorToString(int error);

    // Returns true if |error| is a certificate error code.
    inline bool IsCertificateError(int error)
    {
        // Certificate errors are negative integers from net::ERR_CERT_BEGIN
        // (inclusive) to net::ERR_CERT_END (exclusive) in *decreasing* order.
        return error<=ERR_CERT_BEGIN && error>ERR_CERT_END;
    }

    // Map system error code to Error.
    Error MapSystemError(int os_error);

} //namespace net

#endif //__wan_chrome_net_net_error_h__