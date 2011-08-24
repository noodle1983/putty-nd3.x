
#ifndef __view_tabbed_pane_listener_h__
#define __view_tabbed_pane_listener_h__

#pragma once

namespace view
{

    // An interface implemented by an object to let it know that a tabbed pane was
    // selected by the user at the specified index.
    class TabbedPaneListener
    {
    public:
        // Called when the tab at |index| is selected by the user.
        virtual void TabSelectedAt(int index) = 0;
    };

} //namespace view

#endif //__view_tabbed_pane_listener_h__