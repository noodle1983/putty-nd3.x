
#ifndef __desktop_window_root_view_h__
#define __desktop_window_root_view_h__

#pragma once

#include "view/widget/root_view.h"

namespace view
{
    namespace desktop
    {

        class DesktopWindowView;

        class DesktopWindowRootView : public internal::RootView
        {
        public:
            DesktopWindowRootView(DesktopWindowView* desktop_window_view, Widget* window);
            virtual ~DesktopWindowRootView();

        private:
            // Overridden from RootView:
            virtual bool OnMousePressed(const MouseEvent& event);

             DesktopWindowView* desktop_window_view_;

            DISALLOW_COPY_AND_ASSIGN(DesktopWindowRootView);
        };

    } //namespace desktop
} //namespace view

#endif //__desktop_window_root_view_h__