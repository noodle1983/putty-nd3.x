
#include "sha1.h"

#include "base/string_util.h"

#include "scoped_capi_types.h"

namespace base
{

    std::string SHA1HashString(const std::string& str)
    {
        ScopedHCRYPTPROV provider;
        if(!CryptAcquireContext(provider.receive(), NULL, NULL,
            PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        {
            LOG(ERROR) << "CryptAcquireContext failed: " << GetLastError();
            return std::string(SHA1_LENGTH, '\0');
        }

        {
            ScopedHCRYPTHASH hash;
            if(!CryptCreateHash(provider, CALG_SHA1, 0, 0, hash.receive()))
            {
                LOG(ERROR) << "CryptCreateHash failed: " << GetLastError();
                return std::string(SHA1_LENGTH, '\0');
            }

            if(!CryptHashData(hash, reinterpret_cast<CONST BYTE*>(str.data()),
                static_cast<DWORD>(str.length()), 0))
            {
                LOG(ERROR) << "CryptHashData failed: " << GetLastError();
                return std::string(SHA1_LENGTH, '\0');
            }

            DWORD hash_len = 0;
            DWORD buffer_size = sizeof hash_len;
            if(!CryptGetHashParam(hash, HP_HASHSIZE,
                reinterpret_cast<unsigned char*>(&hash_len),
                &buffer_size, 0))
            {
                LOG(ERROR) << "CryptGetHashParam(HP_HASHSIZE) failed: "
                    << GetLastError();
                return std::string(SHA1_LENGTH, '\0');
            }

            std::string result;
            // +1不是因为函数调用要在结尾写入\0, 而是为了确保result.length()
            // 设置为|hash_len|.
            if(!CryptGetHashParam(hash, HP_HASHVAL, reinterpret_cast<BYTE*>(
                WriteInto(&result, hash_len+1)), &hash_len, 0))
            {
                LOG(ERROR) << "CryptGetHashParam(HP_HASHVAL) failed: "
                    << GetLastError();
                return std::string(SHA1_LENGTH, '\0');
            }

            if(hash_len != SHA1_LENGTH)
            {
                LOG(ERROR) << "Returned hash value is wrong length: "
                    << hash_len << " should be " << SHA1_LENGTH;
                return std::string(SHA1_LENGTH, '\0');
            }

            return result;
        }
    }

    void SHA1HashBytes(const char* data, size_t len, char* hash)
    {
        std::string sha = SHA1HashString(std::string(data, len));
        memcpy(hash, sha.c_str(), SHA1_LENGTH);
    }

} //namespace base