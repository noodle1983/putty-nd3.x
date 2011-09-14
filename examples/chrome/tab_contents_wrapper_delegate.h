
#ifndef __tab_contents_wrapper_delegate_h__
#define __tab_contents_wrapper_delegate_h__

#pragma once

#include "base/basic_types.h"

class TabContentsWrapper;

// Objects implement this interface to get notified about changes in the
// TabContentsWrapper and to provide necessary functionality.
class TabContentsWrapperDelegate
{
public:
    // Notification that a user's request to install an application has completed.
    virtual void OnDidGetApplicationInfo(TabContentsWrapper* source,
        int32 page_id);

    virtual void SwapTabContents(TabContentsWrapper* old_tc,
        TabContentsWrapper* new_tc) = 0;

protected:
    virtual ~TabContentsWrapperDelegate();
};

#endif //__tab_contents_wrapper_delegate_h__