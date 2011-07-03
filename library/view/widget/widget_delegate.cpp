
#include "widget_delegate.h"

#include <windows.h>

#include "SkBitmap.h"

#include "view/view_delegate.h"
#include "view/window/client_view.h"

namespace view
{

    ////////////////////////////////////////////////////////////////////////////////
    // WidgetDelegate:

    WidgetDelegate::WidgetDelegate() : default_contents_view_(NULL) {}

    void WidgetDelegate::OnWidgetActivated(bool active) {}

    void WidgetDelegate::OnWidgetMove() {}

    void WidgetDelegate::OnDisplayChanged() {}

    void WidgetDelegate::OnWorkAreaChanged() {}

    View* WidgetDelegate::GetInitiallyFocusedView()
    {
        return NULL;
    }

    DialogDelegate* WidgetDelegate::AsDialogDelegate()
    {
        return NULL;
    }

    bool WidgetDelegate::CanResize() const
    {
        return false;
    }

    bool WidgetDelegate::CanMaximize() const
    {
        return false;
    }

    bool WidgetDelegate::CanActivate() const
    {
        return true;
    }

    bool WidgetDelegate::IsModal() const
    {
        return false;
    }

    ui::AccessibilityTypes::Role WidgetDelegate::GetAccessibleWindowRole() const
    {
        return ui::AccessibilityTypes::ROLE_WINDOW;
    }

    ui::AccessibilityTypes::State WidgetDelegate::GetAccessibleWindowState() const
    {
        return 0;
    }

    std::wstring WidgetDelegate::GetAccessibleWindowTitle() const
    {
        return GetWindowTitle();
    }

    std::wstring WidgetDelegate::GetWindowTitle() const
    {
        return L"";
    }

    bool WidgetDelegate::ShouldShowWindowTitle() const
    {
        return true;
    }

    bool WidgetDelegate::ShouldShowClientEdge() const
    {
        return true;
    }

    SkBitmap WidgetDelegate::GetWindowAppIcon()
    {
        // Use the window icon as app icon by default.
        return GetWindowIcon();
    }

    // Returns the icon to be displayed in the window.
    SkBitmap WidgetDelegate::GetWindowIcon()
    {
        return SkBitmap();
    }

    bool WidgetDelegate::ShouldShowWindowIcon() const
    {
        return false;
    }

    bool WidgetDelegate::ExecuteWindowsCommand(int command_id)
    {
        return false;
    }

    std::wstring WidgetDelegate::GetWindowName() const
    {
        return std::wstring();
    }

    void WidgetDelegate::SaveWindowPlacement(const gfx::Rect& bounds,
        bool maximized)
    {
        std::wstring window_name = GetWindowName();
        if(!ViewDelegate::view_delegate || window_name.empty())
        {
            return;
        }

        ViewDelegate::view_delegate->SaveWindowPlacement(
            GetWidget(), window_name, bounds, maximized);
    }

    bool WidgetDelegate::GetSavedWindowBounds(gfx::Rect* bounds) const
    {
        std::wstring window_name = GetWindowName();
        if(!ViewDelegate::view_delegate || window_name.empty())
        {
            return false;
        }

        return ViewDelegate::view_delegate->GetSavedWindowBounds(
            GetWidget(), window_name, bounds);
    }

    bool WidgetDelegate::GetSavedMaximizedState(bool* maximized) const
    {
        std::wstring window_name = GetWindowName();
        if(!ViewDelegate::view_delegate || window_name.empty())
        {
            return false;
        }

        return ViewDelegate::view_delegate->GetSavedMaximizedState(
            GetWidget(), window_name, maximized);
    }

    bool WidgetDelegate::ShouldRestoreWindowSize() const
    {
        return true;
    }

    View* WidgetDelegate::GetContentsView()
    {
        if(!default_contents_view_)
        {
            default_contents_view_ = new View;
        }
        return default_contents_view_;
    }

    ClientView* WidgetDelegate::CreateClientView(Widget* widget)
    {
        return new ClientView(widget, GetContentsView());
    }

    NonClientFrameView* WidgetDelegate::CreateNonClientFrameView()
    {
        return NULL;
    }

    bool WidgetDelegate::WillProcessWorkAreaChange() const
    {
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // WidgetDelegateView:

    WidgetDelegateView::WidgetDelegateView() {}

    WidgetDelegateView::~WidgetDelegateView() {}

    Widget* WidgetDelegateView::GetWidget()
    {
        return View::GetWidget();
    }

    const Widget* WidgetDelegateView::GetWidget() const
    {
        return View::GetWidget();
    }

} //namespace view