
#ifndef __demo_view_delegate_h__
#define __demo_view_delegate_h__

#pragma once

#include "base/memory/scoped_ptr.h"
#include "view/view_delegate.h"

class DemoViewDelegate : public view::ViewDelegate
{
public:
    DemoViewDelegate();
    virtual ~DemoViewDelegate();

    void set_default_parent_view(view::View* view)
    {
        default_parent_view_ = view;
    }

    // Overridden from view::ViewDelegate:
    virtual ui::Clipboard* GetClipboard() const;
    virtual view::View* GetDefaultParentView();
    virtual void SaveWindowPlacement(
        const view::Widget* widget,
        const std::wstring& window_name,
        const gfx::Rect& bounds,
        ui::WindowShowState show_state);
    virtual bool GetSavedWindowPlacement(
        const std::wstring& window_name,
        gfx::Rect* bounds,
        ui::WindowShowState* show_state) const;

    virtual void NotifyAccessibilityEvent(view::View* view,
        ui::AccessibilityTypes::Event event_type) {}

    virtual void NotifyMenuItemFocused(
        const std::wstring& menu_name,
        const std::wstring& menu_item_name,
        int item_index,
        int item_count,
        bool has_submenu) {}

    virtual HICON GetDefaultWindowIcon() const;

    virtual void AddRef() {}
    virtual void ReleaseRef() {}

    virtual int GetDispositionForEvent(int event_flags);

private:
    view::View* default_parent_view_;
    mutable scoped_ptr<ui::Clipboard> clipboard_;

    DISALLOW_COPY_AND_ASSIGN(DemoViewDelegate);
};

#endif //__demo_view_delegate_h__