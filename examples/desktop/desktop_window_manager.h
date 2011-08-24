
#ifndef __desktop_window_manager_h__
#define __desktop_window_manager_h__

#pragma once

#include "base/memory/scoped_ptr.h"
#include "view/widget/window_manager.h"

namespace gfx
{
    class Point;
}

namespace view
{
    class Widget;

    namespace desktop
    {
        class WindowController;

        // A tentative window manager for views destktop until we have *right*
        // implementation based on aura/layer API. This is minimum
        // implmenetation and complicated actio like moving transformed window
        // doesn't work.  TODO(oshima): move active widget to WindowManager.
        class DesktopWindowManager : public view::WindowManager
        {
        public:
            DesktopWindowManager(Widget* desktop);
            virtual ~DesktopWindowManager();

            // view::WindowManager implementations:
            virtual void StartMoveDrag(view::Widget* widget,
                const gfx::Point& point);
            virtual void StartResizeDrag(view::Widget* widget,
                const gfx::Point& point,
                int hittest_code);
            virtual bool SetMouseCapture(view::Widget* widget);
            virtual bool ReleaseMouseCapture(view::Widget* widget);
            virtual bool HasMouseCapture(const view::Widget* widget) const;
            virtual bool HandleMouseEvent(view::Widget* widget,
                const view::MouseEvent& event);

        private:
            void SetMouseCapture();
            void ReleaseMouseCapture();
            bool HasMouseCapture() const;

            view::Widget* desktop_;
            view::Widget* mouse_capture_;
            scoped_ptr<WindowController> window_controller_;

            DISALLOW_COPY_AND_ASSIGN(DesktopWindowManager);
        };

        // An behavioral interface for objects implements window resize/movement.
        class WindowController
        {
        public:
            WindowController();
            virtual ~WindowController();
            virtual bool OnMouseEvent(const view::MouseEvent& event) = 0;

        private:
            DISALLOW_COPY_AND_ASSIGN(WindowController);
        };

    } //namespace desktop
} //namespace view

#endif //__desktop_window_manager_h__