
#include "page_navigator.h"

OpenURLParams::OpenURLParams(const Url& url,
                             const Url& referrer,
                             WindowOpenDisposition disposition)
                             : url(url),
                             referrer(referrer),
                             disposition(disposition) {}

OpenURLParams::OpenURLParams() {}

OpenURLParams::~OpenURLParams() {}