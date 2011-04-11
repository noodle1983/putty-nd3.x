
#include "native_scroll_bar.h"

#include "native_scroll_bar_wrapper.h"

namespace view
{

    // static
    const char NativeScrollBar::kViewClassName[] = "view/NativeScrollBar";

    ////////////////////////////////////////////////////////////////////////////////
    // NativeScrollBar, public:
    NativeScrollBar::NativeScrollBar(bool is_horizontal)
        : ScrollBar(is_horizontal), native_wrapper_(NULL) {}

    NativeScrollBar::~NativeScrollBar() {}

    // static
    int NativeScrollBar::GetHorizontalScrollBarHeight()
    {
        return NativeScrollBarWrapper::GetHorizontalScrollBarHeight();
    }

    // static
    int NativeScrollBar::GetVerticalScrollBarWidth()
    {
        return NativeScrollBarWrapper::GetVerticalScrollBarWidth();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeScrollBar, View overrides:
    gfx::Size NativeScrollBar::GetPreferredSize()
    {
        if(native_wrapper_)
        {
            return native_wrapper_->GetView()->GetPreferredSize();
        }
        return gfx::Size();
    }

    void NativeScrollBar::Layout()
    {
        if(native_wrapper_)
        {
            native_wrapper_->GetView()->Layout();
        }
    }

    void NativeScrollBar::ViewHierarchyChanged(bool is_add, View* parent,
        View* child)
    {
        Widget* widget;
        if(is_add && !native_wrapper_ && (widget=GetWidget()))
        {
            native_wrapper_ = NativeScrollBarWrapper::CreateWrapper(this);
            AddChildView(native_wrapper_->GetView());
        }
    }

    std::string NativeScrollBar::GetClassName() const
    {
        return kViewClassName;
    }

    // Overridden from View for keyboard UI.
    bool NativeScrollBar::OnKeyPressed(const KeyEvent& event)
    {
        if(!native_wrapper_)
        {
            return false;
        }
        return native_wrapper_->GetView()->OnKeyPressed(event);
    }

    bool NativeScrollBar::OnMouseWheel(const MouseWheelEvent& event)
    {
        if(!native_wrapper_)
        {
            return false;
        }
        return native_wrapper_->GetView()->OnMouseWheel(event);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeScrollBar, ScrollBar overrides:
    void NativeScrollBar::Update(int viewport_size,
        int content_size,
        int current_pos)
    {
        ScrollBar::Update(viewport_size, content_size, current_pos);

        if(native_wrapper_)
        {
            native_wrapper_->Update(viewport_size, content_size, current_pos);
        }
    }

    int NativeScrollBar::GetLayoutSize() const
    {
        return IsHorizontal() ? GetHorizontalScrollBarHeight() :
            GetVerticalScrollBarWidth();
    }

    int NativeScrollBar::GetPosition() const
    {
        if(!native_wrapper_)
        {
            return 0;
        }
        return native_wrapper_->GetPosition();
    }

} //namespace view