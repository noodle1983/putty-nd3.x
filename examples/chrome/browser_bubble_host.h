
#ifndef __browser_bubble_host_h__
#define __browser_bubble_host_h__

#pragma once

#include <set>

#include "base/basic_types.h"

class BrowserBubble;

// A class providing a hosting environment for BrowserBubble instances.
// Allows for notification to attached BrowserBubbles of browser move, and
// close events.
class BrowserBubbleHost
{
public:
    BrowserBubbleHost();
    ~BrowserBubbleHost();

    // Invoked when the window containing the attached browser-bubbles is moved.
    // Calls BrowserBubble::BrowserWindowMoved on all attached bubbles.
    void WindowMoved();

    // To be called when the frame containing the BrowserBubbleHost is closing.
    // Calls BrowserBubble::BrowserWindowClosing on all attached bubbles.
    void Close();

    // Registers/Unregisters |bubble| to receive notifications when the host moves
    // or is closed.
    void AttachBrowserBubble(BrowserBubble* bubble);
    void DetachBrowserBubble(BrowserBubble* bubble);

private:
    // The set of bubbles associated with this host.
    typedef std::set<BrowserBubble*> BubbleSet;
    BubbleSet browser_bubbles_;

    DISALLOW_COPY_AND_ASSIGN(BrowserBubbleHost);
};

#endif //__browser_bubble_host_h__