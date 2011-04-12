
#ifndef __view_framework_view_constants_h__
#define __view_framework_view_constants_h__

#pragma once

namespace view
{

    // Size (width or height) within which the user can hold the mouse and the
    // view should scroll.
    extern const int kAutoscrollSize;

    // Time in milliseconds to autoscroll by a row. This is used during drag and
    // drop.
    extern const int kAutoscrollRowTimerMS;

    // Used to determine whether a drop is on an item or before/after it. If a drop
    // occurs kDropBetweenPixels from the top/bottom it is considered before/after
    // the item, otherwise it is on the item.
    extern const int kDropBetweenPixels;

} //namespace view

#endif //__view_framework_view_constants_h__