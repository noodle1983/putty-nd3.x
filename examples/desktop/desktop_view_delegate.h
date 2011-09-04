
#ifndef __desktop_view_delegate_h__
#define __desktop_view_delegate_h__

#pragma once

#include "ui_base/ui_base_types.h"

#include "view/view_delegate.h"

namespace view
{
    namespace desktop
    {

        class DesktopViewDelegate : public ViewDelegate
        {
        public:
            DesktopViewDelegate();
            virtual ~DesktopViewDelegate();

        private:
            // Overridden from ViewDelegate:
            virtual ui::Clipboard* GetClipboard() const;
            virtual View* GetDefaultParentView();
            virtual void SaveWindowPlacement(
                const view::Widget* widget,
                const std::wstring& window_name,
                const gfx::Rect& bounds,
                ui::WindowShowState show_state);
            virtual bool GetSavedWindowPlacement(
                const std::wstring& window_name,
                gfx::Rect* bounds,
                ui::WindowShowState* show_state) const;
            virtual void NotifyAccessibilityEvent(
                View* view, ui::AccessibilityTypes::Event event_type);
            virtual void NotifyMenuItemFocused(
                const std::wstring& menu_name,
                const std::wstring& menu_item_name,
                int item_index,
                int item_count,
                bool has_submenu);
            virtual HICON GetDefaultWindowIcon() const;
            virtual void AddRef();
            virtual void ReleaseRef();
            virtual int GetDispositionForEvent(int event_flags);

            DISALLOW_COPY_AND_ASSIGN(DesktopViewDelegate);
        };

    } //namespace desktop
} //namespace view

#endif //__desktop_view_delegate_h__