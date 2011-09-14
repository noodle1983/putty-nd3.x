
#ifndef __bookmark_tab_helper_delegate_h__
#define __bookmark_tab_helper_delegate_h__

#pragma once

#include "base/basic_types.h"

class TabContentsWrapper;

// Objects implement this interface to get notified about changes in the
// BookmarkTabHelper and to provide necessary functionality.
class BookmarkTabHelperDelegate
{
public:
    // Notification that the starredness of the current URL changed.
    virtual void URLStarredChanged(TabContentsWrapper* source, bool starred) = 0;

protected:
    virtual ~BookmarkTabHelperDelegate();
};

#endif //__bookmark_tab_helper_delegate_h__