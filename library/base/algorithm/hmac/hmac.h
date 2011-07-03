
// Utility class for calculating the HMAC for a given message. We currently
// only support SHA1 for the hash algorithm, but this can be extended easily.

#ifndef __rfc_algorithm_hmac_h__
#define __rfc_algorithm_hmac_h__

#pragma once

#include <string>

#include "base/basic_types.h"
#include "base/memory/scoped_ptr.h"

namespace base
{

    // Simplify the interface and reduce includes by abstracting out the internals.
    struct HMACPlatformData;

    class HMAC
    {
    public:
        // The set of supported hash functions. Extend as required.
        enum HashAlgorithm
        {
            SHA1,
            SHA256,
        };

        explicit HMAC(HashAlgorithm hash_alg);
        ~HMAC();

        // Initializes this instance using |key| of the length |key_length|. Call Init
        // only once. It returns false on the second or later calls.
        bool Init(const unsigned char* key, int key_length);

        // Initializes this instance using |key|. Call Init only once. It returns
        // false on the second or later calls.
        bool Init(const std::string& key)
        {
            return Init(reinterpret_cast<const unsigned char*>(key.data()),
                static_cast<int>(key.size()));
        }

        // Calculates the HMAC for the message in |data| using the algorithm supplied
        // to the constructor and the key supplied to the Init method. The HMAC is
        // returned in |digest|, which has |digest_length| bytes of storage available.
        bool Sign(const std::string& data, unsigned char* digest, int digest_length);

        // TODO(albertb): Add a Verify method.

    private:
        HashAlgorithm hash_alg_;
        scoped_ptr<HMACPlatformData> plat_;

        DISALLOW_COPY_AND_ASSIGN(HMAC);
    };

} //namespace base

#endif //__rfc_algorithm_hmac_h__