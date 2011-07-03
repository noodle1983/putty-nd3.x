
#include "desktop_window_root_view.h"

#include "view/widget/native_widget_view.h"

#include "desktop_window_view.h"

namespace view
{
    namespace desktop
    {

        ////////////////////////////////////////////////////////////////////////////////
        // DesktopWindowRootView, public:

        DesktopWindowRootView::DesktopWindowRootView(
            DesktopWindowView* desktop_window_view,
            Widget* window)
            : internal::RootView(window),
            desktop_window_view_(desktop_window_view) {}

        DesktopWindowRootView::~DesktopWindowRootView() {}

        ////////////////////////////////////////////////////////////////////////////////
        // DesktopWindowRootView, internal::RootView overrides:

        bool DesktopWindowRootView::OnMousePressed(const MouseEvent& event)
        {
            View* target = GetEventHandlerForPoint(event.location());
            if(target->GetClassName() == internal::NativeWidgetView::kViewClassName)
            {
                internal::NativeWidgetView* native_widget_view =
                    static_cast<internal::NativeWidgetView*>(target);
                desktop_window_view_->ActivateWidget(
                    native_widget_view->GetAssociatedWidget());
            }
            else
            {
                desktop_window_view_->ActivateWidget(NULL);
            }
            return RootView::OnMousePressed(event);
        }

    } //namespace desktop
} //namespace view