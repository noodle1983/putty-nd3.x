
#ifndef __desktop_window_view_h__
#define __desktop_window_view_h__

#pragma once

#include "view/view.h"
#include "view/widget/widget.h"
#include "view/widget/widget_delegate.h"

namespace view
{
    class NativeWidgetViews;
    class Widget;

    namespace desktop
    {

        class DesktopWindowView : public WidgetDelegateView
        {
        public:
            // Observers can listen to various events on the desktop.
            class Observer
            {
            public:
                virtual void OnDesktopBoundsChanged(
                    const gfx::Rect& previous_bounds) = 0;

            protected:
                virtual ~Observer() {}
            };

            // The look and feel will be slightly different for different kinds of
            // desktop.
            enum DesktopType
            {
                DESKTOP_DEFAULT,
                DESKTOP_NETBOOK,
                DESKTOP_OTHER
            };

            static DesktopWindowView* desktop_window_view;

            DesktopWindowView(DesktopType type);
            virtual ~DesktopWindowView();

            static void CreateDesktopWindow(DesktopType type);

            void CreateTestWindow(const std::wstring& title,
                SkColor color,
                gfx::Rect initial_bounds,
                bool rotate);

            DesktopType type() const { return type_; }

            // Add/remove observer.
            void AddObserver(Observer* observer);
            void RemoveObserver(Observer* observer);
            bool HasObserver(Observer* observer);

        private:
            // Overridden from View:
            virtual void Layout();
            virtual void OnBoundsChanged(const gfx::Rect& previous_bounds);
            virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);

            // Overridden from WidgetDelegate:
            virtual Widget* GetWidget();
            virtual const Widget* GetWidget() const;
            virtual bool CanResize() const;
            virtual bool CanMaximize() const;
            virtual std::wstring GetWindowTitle() const;
            virtual SkBitmap GetWindowAppIcon();
            virtual SkBitmap GetWindowIcon();
            virtual bool ShouldShowWindowIcon() const;
            virtual void WindowClosing();
            virtual View* GetContentsView();
            virtual NonClientFrameView* CreateNonClientFrameView();

            ObserverList<Observer> observers_;
            DesktopType type_;
            Widget* widget_;

            DISALLOW_COPY_AND_ASSIGN(DesktopWindowView);
        };

    } //namespace desktop
} //namespace view

#endif //__desktop_window_view_h__