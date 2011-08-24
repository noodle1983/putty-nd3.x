
#ifndef __view_native_scroll_bar_wrapper_h__
#define __view_native_scroll_bar_wrapper_h__

#pragma once

namespace view
{

    class NativeScrollBar;
    class View;

    // A specialization of NativeControlWrapper that hosts a platform-native
    // scroll bar.
    class NativeScrollBarWrapper
    {
    public:
        virtual ~NativeScrollBarWrapper() {}

        // Updates the scroll bar appearance given a viewport size, content size and
        // current position.
        virtual void Update(int viewport_size, int content_size, int current_pos) = 0;

        // Retrieves the view::View that hosts the native control.
        virtual View* GetView() = 0;

        // Returns the position of the scrollbar.
        virtual int GetPosition() const = 0;

        // Creates an appropriate NativeScrollBarWrapper for the platform.
        static NativeScrollBarWrapper* CreateWrapper(NativeScrollBar* button);

        // Returns the system sizes of vertical/horizontal scroll bars.
        static int GetVerticalScrollBarWidth();
        static int GetHorizontalScrollBarHeight();
    };

} //namespace view

#endif //__view_native_scroll_bar_wrapper_h__