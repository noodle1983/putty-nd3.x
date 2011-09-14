
#include "url.h"

namespace
{

    static std::string* empty_string = NULL;
    static Url* empty_gurl = NULL;

}

Url::Url() {}

Url& Url::operator=(const Url& other)
{
    url_ = other.url_;
    return *this;
}

const Url& Url::EmptyGURL()
{
    // Avoid static object construction/destruction on startup/shutdown.
    if(!empty_gurl)
    {
        // Create the string. Be careful that we don't break in the case that this
        // is being called from multiple threads.
        Url* new_empty_gurl = new Url;
        if(InterlockedCompareExchangePointer(
            reinterpret_cast<PVOID*>(&empty_gurl), new_empty_gurl, NULL))
        {
            // The old value was non-NULL, so no replacement was done. Another
            // thread did the initialization out from under us.
            delete new_empty_gurl;
        }
    }
    return *empty_gurl;
}