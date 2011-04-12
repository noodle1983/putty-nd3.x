
#ifndef __view_native_scroll_bar_h__
#define __view_native_scroll_bar_h__

#pragma once

#include "scroll_bar.h"

namespace view
{

    class NativeScrollBarWrapper;

    // The NativeScrollBar class is a scrollbar that uses platform's
    // native control.
    class NativeScrollBar : public ScrollBar
    {
    public:
        // The scroll-bar's class name.
        static const char kViewClassName[];

        // Create new scrollbar, either horizontal or vertical.
        explicit NativeScrollBar(bool is_horiz);
        virtual ~NativeScrollBar();

        // Return the system sizes.
        static int GetHorizontalScrollBarHeight();
        static int GetVerticalScrollBarWidth();

    private:
        // Overridden from View.
        virtual gfx::Size GetPreferredSize();
        virtual void Layout();
        virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
        virtual std::string GetClassName() const;

        // Overrideen from View for keyboard UI purpose.
        virtual bool OnKeyPressed(const KeyEvent& event);
        virtual bool OnMouseWheel(const MouseWheelEvent& e);

        // Overridden from ScrollBar.
        virtual void Update(int viewport_size, int content_size, int current_pos);
        virtual int GetPosition() const;
        virtual int GetLayoutSize() const;

        // init border
        NativeScrollBarWrapper* native_wrapper_;

        DISALLOW_COPY_AND_ASSIGN(NativeScrollBar);
    };

} //namespace view

#endif //__view_native_scroll_bar_h__