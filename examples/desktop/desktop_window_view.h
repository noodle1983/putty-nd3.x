
#ifndef __desktop_window_view_h__
#define __desktop_window_view_h__

#pragma once

#include "view/view.h"
#include "view/widget/widget_delegate.h"

namespace view
{
    class NativeWidgetViews;

    namespace desktop
    {

        class DesktopWindowView : public WidgetDelegateView
        {
        public:
            static DesktopWindowView* desktop_window_view;

            DesktopWindowView();
            virtual ~DesktopWindowView();

            static void CreateDesktopWindow();

            // Changes activation to the specified Widget. The currently active Widget
            // is de-activated.
            void ActivateWidget(Widget* widget);

            void CreateTestWindow(const std::wstring& title,
                SkColor color,
                gfx::Rect initial_bounds,
                bool rotate);

        private:
            // Overridden from View:
            virtual void Layout();

            // Overridden from WidgetDelegate:
            virtual bool CanResize() const;
            virtual bool CanMaximize() const;
            virtual std::wstring GetWindowTitle() const;
            virtual SkBitmap GetWindowAppIcon();
            virtual SkBitmap GetWindowIcon();
            virtual bool ShouldShowWindowIcon() const;
            virtual void WindowClosing();
            virtual View* GetContentsView();

            NativeWidgetViews* active_widget_;

            DISALLOW_COPY_AND_ASSIGN(DesktopWindowView);
        };

    } //namespace desktop
} //namespace view

#endif //__desktop_window_view_h__