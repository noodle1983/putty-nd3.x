
#include "listbox.h"

#include "../../layout/fill_layout.h"
#include "../native/native_view_host.h"
#include "native_listbox_wrapper.h"

namespace view
{

    // static
    const char Listbox::kViewClassName[] = "view/Listbox";

    ////////////////////////////////////////////////////////////////////////////////
    // Listbox, public:

    Listbox::Listbox(const std::vector<string16>& strings,
        Listbox::Listener* listener)
        : strings_(strings),
        listener_(listener),
        native_wrapper_(NULL)
    {
        SetFocusable(true);
    }

    Listbox::~Listbox() {}

    int Listbox::GetRowCount() const
    {
        return static_cast<int>(strings_.size());
    }

    int Listbox::SelectedRow() const
    {
        if(!native_wrapper_)
        {
            return -1;
        }
        return native_wrapper_->SelectedRow();
    }

    void Listbox::SelectRow(int model_row)
    {
        if(!native_wrapper_)
        {
            return;
        }
        native_wrapper_->SelectRow(model_row);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Listbox, View overrides:

    gfx::Size Listbox::GetPreferredSize()
    {
        if(native_wrapper_)
        {
            return native_wrapper_->GetPreferredSize();
        }
        return gfx::Size();
    }

    void Listbox::Layout()
    {
        if(native_wrapper_)
        {
            native_wrapper_->GetView()->SetBounds(0, 0, width(), height());
            native_wrapper_->GetView()->Layout();
        }
    }

    void Listbox::SetEnabled(bool flag)
    {
        View::SetEnabled(flag);
        if(native_wrapper_)
        {
            native_wrapper_->UpdateEnabled();
        }
    }

    void Listbox::OnPaintFocusBorder(gfx::Canvas* canvas)
    {
        if(NativeViewHost::kRenderNativeControlFocus)
        {
            View::OnPaintFocusBorder(canvas);
        }
    }

    void Listbox::OnFocus()
    {
        // Forward the focus to the wrapper.
        if(native_wrapper_)
        {
            native_wrapper_->SetFocus();
        }
        else
        {
            View::OnFocus(); // Will focus the RootView window (so we still get
                             // keyboard messages).
        }
    }

    void Listbox::ViewHierarchyChanged(bool is_add, View* parent, View* child)
    {
        if(is_add && !native_wrapper_ && GetWidget())
        {
            // The native wrapper's lifetime will be managed by the view hierarchy after
            // we call AddChildView.
            native_wrapper_ = CreateWrapper();
            AddChildView(native_wrapper_->GetView());
        }
    }

    std::string Listbox::GetClassName() const
    {
        return kViewClassName;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Listbox, protected:

    NativeListboxWrapper* Listbox::CreateWrapper()
    {
        return NativeListboxWrapper::CreateNativeWrapper(this, strings_, listener_);
    }

} //namespace view