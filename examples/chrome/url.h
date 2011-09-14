
#ifndef __url_h__
#define __url_h__

#include <windows.h>

#include "base/string16.h"

class Url
{
public:
    Url();
    Url& operator=(const Url& other);

    // Defiant equality operator!
    bool operator==(const Url& other) const
    {
        return url_ == other.url_;
    }
    bool operator!=(const Url& other) const
    {
        return url_ != other.url_;
    }

    // Returns a reference to a singleton empty GURL. This object is for callers
    // who return references but don't have anything to return in some cases.
    // This function may be called from any thread.
    static const Url& EmptyGURL();

private:
    string16 url_;
};

#endif //__url_h__