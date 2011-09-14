
// PageNavigator defines an interface that can be used to express the user's
// intention to navigate to a particular URL.  The implementing class should
// perform the navigation.

#ifndef __page_navigator_h__
#define __page_navigator_h__

#pragma once

#include <string>

#include "url.h"
#include "window_open_disposition.h"

class TabContents;

struct OpenURLParams
{
    OpenURLParams(const Url& url,
        const Url& referrer,
        WindowOpenDisposition disposition);
    ~OpenURLParams();

    // The URL/referrer to be opened.
    Url url;
    Url referrer;

    // The disposition requested by the navigation source.
    WindowOpenDisposition disposition;

    // The override encoding of the URL contents to be opened.
    std::string override_encoding;

private:
    OpenURLParams();
};

class PageNavigator
{
public:
    // Deprecated. Please use the one-argument variant instead.
    // TODO(adriansc): Remove this method when refactoring changed all call sites.
    virtual TabContents* OpenURL(const Url& url,
        const Url& referrer,
        WindowOpenDisposition disposition) = 0;

    // Opens a URL with the given disposition.  The transition specifies how this
    // navigation should be recorded in the history system (for example, typed).
    // Returns the TabContents the URL is opened in, or NULL if the URL wasn't
    // opened immediately.
    virtual TabContents* OpenURL(const OpenURLParams& params) = 0;

protected:
    virtual ~PageNavigator() {}
};

#endif //__page_navigator_h__