
#include "textfield.h"

#include "../../accessibility/accessible_view_state.h"
#include "../../base/range.h"
#include "../../event/event_utils_win.h"
#include "../native/native_view_host.h"
#include "native_textfield_view.h"
#include "native_textfield_win.h"
#include "textfield_controller.h"

namespace view
{

    // static
    const char Textfield::kViewClassName[] = "view/Textfield";

    /////////////////////////////////////////////////////////////////////////////
    // Textfield

    Textfield::Textfield()
        : native_wrapper_(NULL),
        controller_(NULL),
        style_(STYLE_DEFAULT),
        read_only_(false),
        default_width_in_chars_(0),
        draw_border_(true),
        text_color_(SK_ColorBLACK),
        use_default_text_color_(true),
        background_color_(SK_ColorWHITE),
        use_default_background_color_(true),
        num_lines_(1),
        initialized_(false),
        horizontal_margins_were_set_(false),
        vertical_margins_were_set_(false)
    {
        SetFocusable(true);
    }

    Textfield::Textfield(StyleFlags style)
        : native_wrapper_(NULL),
        controller_(NULL),
        style_(style),
        read_only_(false),
        default_width_in_chars_(0),
        draw_border_(true),
        text_color_(SK_ColorBLACK),
        use_default_text_color_(true),
        background_color_(SK_ColorWHITE),
        use_default_background_color_(true),
        num_lines_(1),
        initialized_(false),
        horizontal_margins_were_set_(false),
        vertical_margins_were_set_(false)
    {
        SetFocusable(true);
    }

    Textfield::~Textfield() {}

    void Textfield::SetController(TextfieldController* controller)
    {
        controller_ = controller;
    }

    TextfieldController* Textfield::GetController() const
    {
        return controller_;
    }

    void Textfield::SetReadOnly(bool read_only)
    {
        read_only_ = read_only;
        if(native_wrapper_)
        {
            native_wrapper_->UpdateReadOnly();
            native_wrapper_->UpdateTextColor();
            native_wrapper_->UpdateBackgroundColor();
        }
    }

    bool Textfield::IsPassword() const
    {
        return style_ & STYLE_PASSWORD;
    }

    void Textfield::SetPassword(bool password)
    {
        if(password)
        {
            style_ = static_cast<StyleFlags>(style_|STYLE_PASSWORD);
        }
        else
        {
            style_ = static_cast<StyleFlags>(style_&~STYLE_PASSWORD);
        }
        if(native_wrapper_)
        {
            native_wrapper_->UpdateIsPassword();
        }
    }

    bool Textfield::IsMultiLine() const
    {
        return !!(style_ & STYLE_MULTILINE);
    }

    void Textfield::SetText(const string16& text)
    {
        text_ = text;
        if(native_wrapper_)
        {
            native_wrapper_->UpdateText();
        }
    }

    void Textfield::AppendText(const string16& text)
    {
        text_ += text;
        if(native_wrapper_)
        {
            native_wrapper_->AppendText(text);
        }
    }

    void Textfield::SelectAll()
    {
        if(native_wrapper_)
        {
            native_wrapper_->SelectAll();
        }
    }

    string16 Textfield::GetSelectedText() const
    {
        if(native_wrapper_)
        {
            return native_wrapper_->GetSelectedText();
        }
        return string16();
    }

    void Textfield::ClearSelection() const
    {
        if(native_wrapper_)
        {
            native_wrapper_->ClearSelection();
        }
    }

    bool Textfield::HasSelection() const
    {
        Range range;
        if(native_wrapper_)
        {
            native_wrapper_->GetSelectedRange(&range);
        }
        return !range.is_empty();
    }

    void Textfield::SetTextColor(SkColor color)
    {
        text_color_ = color;
        use_default_text_color_ = false;
        if(native_wrapper_)
        {
            native_wrapper_->UpdateTextColor();
        }
    }

    void Textfield::UseDefaultTextColor()
    {
        use_default_text_color_ = true;
        if(native_wrapper_)
        {
            native_wrapper_->UpdateTextColor();
        }
    }

    void Textfield::SetBackgroundColor(SkColor color)
    {
        background_color_ = color;
        use_default_background_color_ = false;
        if(native_wrapper_)
        {
            native_wrapper_->UpdateBackgroundColor();
        }
    }

    void Textfield::UseDefaultBackgroundColor()
    {
        use_default_background_color_ = true;
        if(native_wrapper_)
        {
            native_wrapper_->UpdateBackgroundColor();
        }
    }

    void Textfield::SetFont(const gfx::Font& font)
    {
        font_ = font;
        if(native_wrapper_)
        {
            native_wrapper_->UpdateFont();
        }
        PreferredSizeChanged();
    }

    void Textfield::SetHorizontalMargins(int left, int right)
    {
        margins_.Set(margins_.top(), left, margins_.bottom(), right);
        horizontal_margins_were_set_ = true;
        if(native_wrapper_)
        {
            native_wrapper_->UpdateHorizontalMargins();
        }
        PreferredSizeChanged();
    }

    void Textfield::SetVerticalMargins(int top, int bottom)
    {
        margins_.Set(top, margins_.left(), bottom, margins_.right());
        vertical_margins_were_set_ = true;
        if(native_wrapper_)
        {
            native_wrapper_->UpdateVerticalMargins();
        }
        PreferredSizeChanged();
    }

    void Textfield::SetHeightInLines(int num_lines)
    {
        DCHECK(IsMultiLine());
        num_lines_ = num_lines;
        PreferredSizeChanged();
    }

    void Textfield::RemoveBorder()
    {
        if(!draw_border_)
        {
            return;
        }

        draw_border_ = false;
        if(native_wrapper_)
        {
            native_wrapper_->UpdateBorder();
        }
    }

    bool Textfield::GetHorizontalMargins(int* left, int* right)
    {
        if(!horizontal_margins_were_set_)
        {
            return false;
        }
        *left = margins_.left();
        *right = margins_.right();
        return true;
    }

    bool Textfield::GetVerticalMargins(int* top, int* bottom)
    {
        if(!vertical_margins_were_set_)
        {
            return false;
        }
        *top = margins_.top();
        *bottom = margins_.bottom();
        return true;
    }

    void Textfield::UpdateAllProperties()
    {
        if(native_wrapper_)
        {
            native_wrapper_->UpdateText();
            native_wrapper_->UpdateTextColor();
            native_wrapper_->UpdateBackgroundColor();
            native_wrapper_->UpdateReadOnly();
            native_wrapper_->UpdateFont();
            native_wrapper_->UpdateEnabled();
            native_wrapper_->UpdateBorder();
            native_wrapper_->UpdateIsPassword();
            native_wrapper_->UpdateHorizontalMargins();
            native_wrapper_->UpdateVerticalMargins();
        }
    }

    void Textfield::SyncText()
    {
        if(native_wrapper_)
        {
            string16 new_text = native_wrapper_->GetText();
            if(new_text != text_)
            {
                text_ = new_text;
                if(controller_)
                {
                    controller_->ContentsChanged(this, text_);
                }
            }
        }
    }

    bool Textfield::IsIMEComposing() const
    {
        return native_wrapper_ && native_wrapper_->IsIMEComposing();
    }

    void Textfield::GetSelectedRange(Range* range) const
    {
        DCHECK(native_wrapper_);
        native_wrapper_->GetSelectedRange(range);
    }

    void Textfield::SelectRange(const Range& range)
    {
        DCHECK(native_wrapper_);
        native_wrapper_->SelectRange(range);
    }

    size_t Textfield::GetCursorPosition() const
    {
        DCHECK(native_wrapper_);
        return native_wrapper_->GetCursorPosition();
    }

    void Textfield::SetAccessibleName(const string16& name)
    {
        accessible_name_ = name;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Textfield, View overrides:

    void Textfield::Layout()
    {
        if(native_wrapper_)
        {
            native_wrapper_->GetView()->SetBoundsRect(GetLocalBounds());
            native_wrapper_->GetView()->Layout();
        }
    }

    gfx::Size Textfield::GetPreferredSize()
    {
        gfx::Insets insets;
        if(draw_border_ && native_wrapper_)
        {
            insets = native_wrapper_->CalculateInsets();
        }
        return gfx::Size(font_.GetExpectedTextWidth(default_width_in_chars_)+
            insets.width(), num_lines_*font_.GetHeight()+insets.height());
    }

    bool Textfield::IsFocusable() const
    {
        return View::IsFocusable() && !read_only_;
    }

    void Textfield::AboutToRequestFocusFromTabTraversal(bool reverse)
    {
        SelectAll();
    }

    bool Textfield::SkipDefaultKeyEventProcessing(const KeyEvent& e)
    {
        // TODO(hamaji): Figure out which keyboard combinations we need to add here,
        //               similar to LocationBarView::SkipDefaultKeyEventProcessing.
        KeyboardCode key = e.key_code();
        if(key == VKEY_BACK)
        {
            return true; // We'll handle BackSpace ourselves.
        }

        // We don't translate accelerators for ALT + NumPad digit on Windows, they are
        // used for entering special characters.  We do translate alt-home.
        if(e.IsAltDown() && (key!=VKEY_HOME) &&
            NativeTextfieldWin::IsNumPadDigit(key, IsExtendedKey(e)))
        {
            return true;
        }
        return false;
    }

    void Textfield::OnPaintBackground(gfx::Canvas* canvas)
    {
        // Overridden to be public - gtk_views_entry.cc wants to call it.
        View::OnPaintBackground(canvas);
    }

    void Textfield::OnPaintFocusBorder(gfx::Canvas* canvas)
    {
        if(NativeViewHost::kRenderNativeControlFocus)
        {
            View::OnPaintFocusBorder(canvas);
        }
    }

    bool Textfield::OnKeyPressed(const KeyEvent& e)
    {
        return native_wrapper_ && native_wrapper_->HandleKeyPressed(e);
    }

    bool Textfield::OnKeyReleased(const KeyEvent& e)
    {
        return native_wrapper_ && native_wrapper_->HandleKeyReleased(e);
    }

    void Textfield::OnFocus()
    {
        if(native_wrapper_)
        {
            native_wrapper_->HandleFocus();
        }

        // Forward the focus to the wrapper if it exists.
        if(!native_wrapper_ || !native_wrapper_->SetFocus())
        {
            // If there is no wrapper or the wrapper didn't take focus, call
            // View::Focus to clear the native focus so that we still get
            // keyboard messages.
            View::OnFocus();
        }
    }

    void Textfield::OnBlur()
    {
        if(native_wrapper_)
        {
            native_wrapper_->HandleBlur();
        }
    }

    void Textfield::GetAccessibleState(AccessibleViewState* state)
    {
        state->role = AccessibilityTypes::ROLE_TEXT;
        state->name = accessible_name_;
        if(read_only())
        {
            state->state |= AccessibilityTypes::STATE_READONLY;
        }
        if(IsPassword())
        {
            state->state |= AccessibilityTypes::STATE_PROTECTED;
        }
        state->value = text_;

        DCHECK(native_wrapper_);
        Range range;
        native_wrapper_->GetSelectedRange(&range);
        state->selection_start = range.start();
        state->selection_end = range.end();
    }

    TextInputClient* Textfield::GetTextInputClient()
    {
        return native_wrapper_ ? native_wrapper_->GetTextInputClient() : NULL;
    }

    void Textfield::SetEnabled(bool enabled)
    {
        View::SetEnabled(enabled);
        if(native_wrapper_)
        {
            native_wrapper_->UpdateEnabled();
        }
    }

    void Textfield::ViewHierarchyChanged(bool is_add, View* parent, View* child)
    {
        if(is_add && !native_wrapper_ && GetWidget() && !initialized_)
        {
            initialized_ = true;

            // The native wrapper's lifetime will be managed by the view hierarchy after
            // we call AddChildView.
            native_wrapper_ = NativeTextfieldWrapper::CreateWrapper(this);
            AddChildView(native_wrapper_->GetView());
            // TODO(beng): Move this initialization to NativeTextfieldWin once it
            //             subclasses NativeControlWin.
            UpdateAllProperties();

            if(!NativeTextfieldView::IsTextfieldViewEnabled())
            {
                // TODO(beng): remove this once NativeTextfieldWin subclasses
                // NativeControlWin. This is currently called to perform post-AddChildView
                // initialization for the wrapper. The GTK version subclasses things
                // correctly and doesn't need this.
                //
                // Remove the include for native_textfield_win.h above when you fix this.
                static_cast<NativeTextfieldWin*>(native_wrapper_)->AttachHack();
            }
        }
    }

    std::string Textfield::GetClassName() const
    {
        return kViewClassName;
    }

} //namespace view