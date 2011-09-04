
#ifndef __view_window_manager_h__
#define __view_window_manager_h__

#pragma once

#include "base/basic_types.h"

namespace gfx
{
    class Point;
}

namespace view
{
    class KeyEvent;
    class MouseEvent;
    class Widget;

    // A interface to WindowManager.
    class WindowManager
    {
    public:
        WindowManager();
        virtual ~WindowManager();

        // Starts moving window given by |widget|. |point| represents the
        // initial location of the mouse pointer.
        virtual void StartMoveDrag(Widget* widget, const gfx::Point& point) = 0;

        // Starts resizing window give by |widget|. |point| represents the
        // initial location of the mouse pointer and |hittest_code| represents
        // the edge of the window a user selected to resize the window. See
        // views/window/hit_test.h for the hittest_code definition.
        virtual void StartResizeDrag(Widget* widget, const gfx::Point& point,
            int hittest_code) = 0;

        // Sets mouse capture on |widget|. Returns false if other widget
        // already has mouse capture.
        virtual bool SetMouseCapture(Widget* widget) = 0;

        // Releases the mouse capture on |widget|. Returns false if |widget|
        // haven't capture the mouse.
        virtual bool ReleaseMouseCapture(Widget* widget) = 0;

        // Checks if the |widget| has mouse capture.
        virtual bool HasMouseCapture(const Widget* widget) const = 0;

        // WindowManager handles mouse event first. It may reisze/move window,
        // or send the event to widget that has mouse capture.
        virtual bool HandleKeyEvent(Widget* widget, const KeyEvent& event) = 0;

        // WindowManager handles mouse event first. It may reisze/move window,
        // or send the event to widget that has mouse capture.
        virtual bool HandleMouseEvent(Widget* widget, const MouseEvent& event) = 0;

        // Register widget to the window manager.
        virtual void Register(Widget* widget) = 0;

        // Installs window manager.
        static void Install(WindowManager* wm);

        // Returns installed WindowManager.
        static WindowManager* Get();

    private:
        DISALLOW_COPY_AND_ASSIGN(WindowManager);
    };

} //namespace view

#endif //__view_window_manager_h__