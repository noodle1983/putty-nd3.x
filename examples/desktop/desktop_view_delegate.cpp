
#include "desktop_view_delegate.h"

#include "base/logging.h"

#include "desktop_window_view.h"

namespace view
{
    namespace desktop
    {

        ////////////////////////////////////////////////////////////////////////////////
        // DesktopViewDelegate, public:

        DesktopViewDelegate::DesktopViewDelegate()
        {
            DCHECK(!ViewDelegate::view_delegate);
            ViewDelegate::view_delegate = this;
        }

        DesktopViewDelegate::~DesktopViewDelegate() {}

        ////////////////////////////////////////////////////////////////////////////////
        // DesktopViewDelegate, ViewDelegate implementation:

        ui::Clipboard* DesktopViewDelegate::GetClipboard() const
        {
            return NULL;
        }

        View* DesktopViewDelegate::GetDefaultParentView()
        {
            return DesktopWindowView::desktop_window_view;
        }

        void DesktopViewDelegate::SaveWindowPlacement(
            const view::Widget* widget,
            const std::wstring& window_name,
            const gfx::Rect& bounds,
            ui::WindowShowState show_state) {}

        bool DesktopViewDelegate::GetSavedWindowPlacement(
            const std::wstring& window_name,
            gfx::Rect* bounds,
            ui::WindowShowState* show_state) const
        {
            return false;
        }

        void DesktopViewDelegate::NotifyAccessibilityEvent(
            View* view, ui::AccessibilityTypes::Event event_type) {}

        void DesktopViewDelegate::NotifyMenuItemFocused(
            const std::wstring& menu_name,
            const std::wstring& menu_item_name,
            int item_index,
            int item_count,
            bool has_submenu) {}

        HICON DesktopViewDelegate::GetDefaultWindowIcon() const
        {
            return NULL;
        }

        void DesktopViewDelegate::AddRef() {}

        void DesktopViewDelegate::ReleaseRef() {}

        int DesktopViewDelegate::GetDispositionForEvent(int event_flags)
        {
            return 0;
        }

    } //namespace desktop
} //namespace view