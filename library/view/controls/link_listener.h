
#ifndef __view_link_listener_h__
#define __view_link_listener_h__

#pragma once

namespace view
{

    class Link;

    // An interface implemented by an object to let it know that a link was clicked.
    class LinkListener
    {
    public:
        virtual void LinkClicked(Link* source, int event_flags) = 0;

    protected:
        virtual ~LinkListener() {}
    };

} //namespace view

#endif //__view_link_listener_h__