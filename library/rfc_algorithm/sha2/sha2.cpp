
#include "sha2.h"

#include "third_party/nss/blapi.h"
#include "third_party/nss/sha256.h"

#include "base/stl_utilinl.h"

namespace base
{

    void SHA256HashString(const std::string& str, void* output, size_t len)
    {
        SHA256Context ctx;

        SHA256_Begin(&ctx);
        SHA256_Update(&ctx, reinterpret_cast<const unsigned char*>(str.data()),
            static_cast<unsigned int>(str.length()));
        SHA256_End(&ctx, static_cast<unsigned char*>(output), NULL,
            static_cast<unsigned int>(len));
    }

    std::string SHA256HashString(const std::string& str)
    {
        std::string output(SHA256_LENGTH, 0);
        SHA256HashString(str, string_as_array(&output), output.size());
        return output;
    }

} //namespace base