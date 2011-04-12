
#include "window_delegate.h"

#include "SkBitmap.h"

#include "../view/view_delegate.h"
#include "../view/client_view.h"
#include "window.h"

namespace view
{

    WindowDelegate::WindowDelegate() : window_(NULL) {}

    WindowDelegate::~WindowDelegate() {}

    DialogDelegate* WindowDelegate::AsDialogDelegate()
    {
        return NULL;
    }

    bool WindowDelegate::CanResize() const
    {
        return false;
    }

    bool WindowDelegate::CanMaximize() const
    {
        return false;
    }

    bool WindowDelegate::CanActivate() const
    {
        return true;
    }

    bool WindowDelegate::IsModal() const
    {
        return false;
    }

    AccessibilityTypes::Role WindowDelegate::GetAccessibleWindowRole() const
    {
        return AccessibilityTypes::ROLE_WINDOW;
    }

    AccessibilityTypes::State WindowDelegate::GetAccessibleWindowState() const
    {
        return 0;
    }

    std::wstring WindowDelegate::GetAccessibleWindowTitle() const
    {
        return GetWindowTitle();
    }

    std::wstring WindowDelegate::GetWindowTitle() const
    {
        return L"";
    }

    View* WindowDelegate::GetInitiallyFocusedView()
    {
        return NULL;
    }

    bool WindowDelegate::ShouldShowWindowTitle() const
    {
        return true;
    }

    bool WindowDelegate::ShouldShowClientEdge() const
    {
        return true;
    }

    SkBitmap WindowDelegate::GetWindowAppIcon()
    {
        // Use the window icon as app icon by default.
        return GetWindowIcon();
    }

    // Returns the icon to be displayed in the window.
    SkBitmap WindowDelegate::GetWindowIcon()
    {
        return SkBitmap();
    }

    bool WindowDelegate::ShouldShowWindowIcon() const
    {
        return false;
    }

    bool WindowDelegate::ExecuteWindowsCommand(int command_id)
    {
        return false;
    }

    std::wstring WindowDelegate::GetWindowName() const
    {
        return std::wstring();
    }

    void WindowDelegate::SaveWindowPlacement(const gfx::Rect& bounds,
        bool maximized)
    {
        std::wstring window_name = GetWindowName();
        if(!ViewDelegate::view_delegate || window_name.empty())
        {
            return;
        }

        ViewDelegate::view_delegate->SaveWindowPlacement(
            window_, window_name, bounds, maximized);
    }

    bool WindowDelegate::GetSavedWindowBounds(gfx::Rect* bounds) const
    {
        std::wstring window_name = GetWindowName();
        if(!ViewDelegate::view_delegate || window_name.empty())
        {
            return false;
        }

        return ViewDelegate::view_delegate->GetSavedWindowBounds(
            window_, window_name, bounds);
    }

    bool WindowDelegate::GetSavedMaximizedState(bool* maximized) const
    {
        std::wstring window_name = GetWindowName();
        if(!ViewDelegate::view_delegate || window_name.empty())
        {
            return false;
        }

        return ViewDelegate::view_delegate->GetSavedMaximizedState(
            window_, window_name, maximized);
    }

    bool WindowDelegate::ShouldRestoreWindowSize() const
    {
        return true;
    }

    View* WindowDelegate::GetContentsView()
    {
        return NULL;
    }

    ClientView* WindowDelegate::CreateClientView(Window* window)
    {
        return new ClientView(window, GetContentsView());
    }

} //namespace view