
#ifndef __view_framework_view_menu_delegate_h__
#define __view_framework_view_menu_delegate_h__

#pragma once

namespace gfx
{
    class Point;
}

namespace view
{

    class View;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // ViewMenuDelegate
    //
    // An interface that allows a component to tell a View about a menu that it
    // has constructed that the view can show (e.g. for MenuButton views, or as a
    // context menu.)
    //
    ////////////////////////////////////////////////////////////////////////////////
    class ViewMenuDelegate
    {
    public:
        // Create and show a menu at the specified position. Source is the view the
        // ViewMenuDelegate was set on.
        virtual void RunMenu(View* source, const gfx::Point& point) = 0;

    protected:
        virtual ~ViewMenuDelegate() {}
    };

} //namespace view

#endif //__view_framework_view_menu_delegate_h__