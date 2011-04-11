
#ifndef __view_framework_metrics_h__
#define __view_framework_metrics_h__

#pragma once

namespace view
{

    // NOTE: All times in this file are/should be expressed in milliseconds.

    // The default value for how long to wait before showing a menu button on hover.
    // This value is used if the OS doesn't supply one.
    extern const int kDefaultMenuShowDelay;

    // Returns the amount of time between double clicks.
    int GetDoubleClickInterval();

    // Returns the amount of time to wait from hovering over a menu button until
    // showing the menu.
    int GetMenuShowDelay();

} //namespace view

#endif //__view_framework_metrics_h__