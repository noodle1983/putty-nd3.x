
#ifndef __bookmark_bar_h__
#define __bookmark_bar_h__

#pragma once

#include "base/basic_types.h"

class BookmarkBar
{
public:
    enum State
    {
        // The bookmark bar is not visible.
        HIDDEN,

        // The bookmark bar is visible and not detached.
        SHOW,

        // The bookmark bar is visible and detached from the location bar (as
        // happens on the new tab page).
        DETACHED
    };

    // Used when the state changes to indicate if the transition should be
    // animated.
    enum AnimateChangeType
    {
        ANIMATE_STATE_CHANGE,
        DONT_ANIMATE_STATE_CHANGE
    };

private:
    DISALLOW_IMPLICIT_CONSTRUCTORS(BookmarkBar);
};

#endif //__bookmark_bar_h__