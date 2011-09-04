
#ifndef __ui_base_types_h__
#define __ui_base_types_h__

#pragma once

namespace ui
{

    // Window "show" state.  These values are written to disk so should not be
    // changed.
    enum WindowShowState
    {
        // A default un-set state.
        SHOW_STATE_DEFAULT    = 0,
        SHOW_STATE_NORMAL     = 1,
        SHOW_STATE_MINIMIZED  = 2,
        SHOW_STATE_MAXIMIZED  = 3,
        SHOW_STATE_INACTIVE   = 4, // Views only, not persisted.
        SHOW_STATE_MAX        = 5
    };

} //namespace ui

#endif //__ui_base_types_h__