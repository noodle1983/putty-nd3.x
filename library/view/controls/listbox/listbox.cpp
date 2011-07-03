
#include "listbox.h"

#include "native_listbox_wrapper.h"
#include "view/controls/native/native_view_host.h"
#include "view/widget/widget.h"

namespace view
{

    // static
    const char Listbox::kViewClassName[] = "view/Listbox";

    ////////////////////////////////////////////////////////////////////////////////
    // Listbox, public:

    Listbox::Listbox(ListboxModel* model)
        : model_(model),
        listener_(NULL),
        native_wrapper_(NULL)
    {
        set_focusable(true);
    }

    Listbox::~Listbox() {}

    void Listbox::ModelChanged()
    {
        if(native_wrapper_)
        {
            native_wrapper_->UpdateFromModel();
        }
        PreferredSizeChanged();
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

    void Listbox::SelectionChanged()
    {
        if(listener_)
        {
            listener_->SelectionChanged(this);
        }
        if(GetWidget())
        {
            GetWidget()->NotifyAccessibilityEvent(this,
                ui::AccessibilityTypes::EVENT_SELECTION_CHANGED, false);
        }
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

    void Listbox::OnEnabledChanged()
    {
        View::OnEnabledChanged();
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
            native_wrapper_ = NativeListboxWrapper::CreateNativeWrapper(this);
            AddChildView(native_wrapper_->GetView());
        }
    }

    std::string Listbox::GetClassName() const
    {
        return kViewClassName;
    }

} //namespace view