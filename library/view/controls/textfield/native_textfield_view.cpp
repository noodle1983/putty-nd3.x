
#include "native_textfield_view.h"

#include "base/command_line.h"

#include "message/message_loop.h"

#include "gfx/canvas_skia.h"

#include "../../base/app_res_ids.h"
#include "../../base/metrics.h"
#include "../../clipboard/clipboard.h"
#include "../../view/view_delegate.h"
#include "../menu/menu_2.h"
#include "textfield.h"
#include "textfield_controller.h"
#include "textfield_view_model.h"

namespace
{

    // A global flag to switch the Textfield wrapper to TextfieldView.
    bool textfield_view_enabled = false;

    // Color setttings for text, border, backgrounds and cursor.
    // These are tentative, and should be derived from theme, system
    // settings and current settings.
    const SkColor kSelectedTextColor = SK_ColorWHITE;
    const SkColor kReadonlyTextColor = SK_ColorDKGRAY;
    const SkColor kFocusedSelectionColor = SK_ColorBLUE;
    const SkColor kUnfocusedSelectionColor = SK_ColorLTGRAY;
    const SkColor kFocusedBorderColor = SK_ColorCYAN;
    const SkColor kDefaultBorderColor = SK_ColorGRAY;
    const SkColor kCursorColor = SK_ColorBLACK;

    // Parameters to control cursor blinking.
    const int kCursorVisibleTimeMs = 800;
    const int kCursorInvisibleTimeMs = 500;

    // A switch to enable NativeTextfieldView;
    const char kEnableViewsBasedTextfieldSwitch[] = "enable-textfield-view";

}

namespace view
{

    const char NativeTextfieldView::kViewClassName[] =
        "view/NativeTextfieldView";

    NativeTextfieldView::NativeTextfieldView(Textfield* parent)
        : textfield_(parent),
        model_(new TextfieldViewsModel()),
        text_border_(new TextfieldBorder()),
        text_offset_(0),
        insert_(true),
        is_cursor_visible_(false),
        cursor_timer_(this),
        last_mouse_press_time_(base::Time::FromInternalValue(0)),
        click_state_(NONE)
    {
        set_border(text_border_);

        // Multiline is not supported.
        DCHECK_NE(parent->style(), Textfield::STYLE_MULTILINE);
        // Lowercase is not supported.
        DCHECK_NE(parent->style(), Textfield::STYLE_LOWERCASE);

        SetContextMenuController(this);
    }

    NativeTextfieldView::~NativeTextfieldView() {}

    ////////////////////////////////////////////////////////////////////////////////
    // NativeTextfieldView, View overrides:

    bool NativeTextfieldView::OnMousePressed(const MouseEvent& e)
    {
        if(HandleMousePressed(e))
        {
            SchedulePaint();
        }
        return true;
    }

    bool NativeTextfieldView::OnMouseDragged(const MouseEvent& e)
    {
        size_t pos = FindCursorPosition(e.location());
        if(model_->MoveCursorTo(pos, true))
        {
            UpdateCursorBoundsAndTextOffset();
            SchedulePaint();
        }
        return true;
    }

    void NativeTextfieldView::OnMouseReleased(const MouseEvent& e,
        bool canceled) {}

    bool NativeTextfieldView::OnKeyPressed(const KeyEvent& e)
    {
        // OnKeyPressed/OnKeyReleased/OnFocus/OnBlur will never be invoked on
        // NativeTextfieldView as it will never gain focus.
        NOTREACHED();
        return false;
    }

    bool NativeTextfieldView::OnKeyReleased(const KeyEvent& e)
    {
        NOTREACHED();
        return false;
    }

    void NativeTextfieldView::OnPaint(gfx::Canvas* canvas)
    {
        text_border_->set_has_focus(textfield_->HasFocus());
        OnPaintBackground(canvas);
        PaintTextAndCursor(canvas);
        if(textfield_->draw_border())
        {
            OnPaintBorder(canvas);
        }
    }

    void NativeTextfieldView::OnFocus()
    {
        NOTREACHED();
    }

    void NativeTextfieldView::OnBlur()
    {
        NOTREACHED();
    }

    HCURSOR NativeTextfieldView::GetCursorForPoint(EventType event_type,
        const gfx::Point& p)
    {
        static HCURSOR ibeam = LoadCursor(NULL, IDC_IBEAM);
        return ibeam;
    }

    /////////////////////////////////////////////////////////////////
    // NativeTextfieldView, views::ContextMenuController overrides:
    void NativeTextfieldView::ShowContextMenuForView(View* source,
        const gfx::Point& p,
        bool is_mouse_gesture)
    {
        InitContextMenuIfRequired();
        context_menu_menu_->RunContextMenuAt(p);
    }

    /////////////////////////////////////////////////////////////////
    // NativeTextfieldView, NativeTextifieldWrapper overrides:

    string16 NativeTextfieldView::GetText() const
    {
        return model_->text();
    }

    void NativeTextfieldView::UpdateText()
    {
        bool changed = model_->SetText(textfield_->text());
        UpdateCursorBoundsAndTextOffset();
        SchedulePaint();
        if(changed)
        {
            TextfieldController* controller = textfield_->GetController();
            if(controller)
            {
                controller->ContentsChanged(textfield_, GetText());
            }
        }
    }

    void NativeTextfieldView::AppendText(const string16& text)
    {
        if(text.empty())
        {
            return;
        }
        model_->Append(text);
        UpdateCursorBoundsAndTextOffset();
        SchedulePaint();

        TextfieldController* controller = textfield_->GetController();
        if(controller)
        {
            controller->ContentsChanged(textfield_, GetText());
        }
    }

    string16 NativeTextfieldView::GetSelectedText() const
    {
        return model_->GetSelectedText();
    }

    void NativeTextfieldView::SelectAll()
    {
        model_->SelectAll();
        SchedulePaint();
    }

    void NativeTextfieldView::ClearSelection()
    {
        model_->ClearSelection();
        SchedulePaint();
    }

    void NativeTextfieldView::UpdateBorder()
    {
        if(textfield_->draw_border())
        {
            gfx::Insets insets = GetInsets();
            textfield_->SetHorizontalMargins(insets.left(), insets.right());
            textfield_->SetVerticalMargins(insets.top(), insets.bottom());
        }
        else
        {
            textfield_->SetHorizontalMargins(0, 0);
            textfield_->SetVerticalMargins(0, 0);
        }
    }

    void NativeTextfieldView::UpdateTextColor()
    {
        SchedulePaint();
    }

    void NativeTextfieldView::UpdateBackgroundColor()
    {
        // TODO(oshima): Background has to match the border's shape.
        set_background(
            Background::CreateSolidBackground(textfield_->background_color()));
        SchedulePaint();
    }

    void NativeTextfieldView::UpdateReadOnly()
    {
        SchedulePaint();
    }

    void NativeTextfieldView::UpdateFont()
    {
        UpdateCursorBoundsAndTextOffset();
    }

    void NativeTextfieldView::UpdateIsPassword()
    {
        model_->set_is_password(textfield_->IsPassword());
        UpdateCursorBoundsAndTextOffset();
        SchedulePaint();
    }

    void NativeTextfieldView::UpdateEnabled()
    {
        SetEnabled(textfield_->IsEnabled());
        SchedulePaint();
    }

    gfx::Insets NativeTextfieldView::CalculateInsets()
    {
        return GetInsets();
    }

    void NativeTextfieldView::UpdateHorizontalMargins()
    {
        int left, right;
        if(!textfield_->GetHorizontalMargins(&left, &right))
        {
            return;
        }
        gfx::Insets inset = GetInsets();

        text_border_->SetInsets(inset.top(), left, inset.bottom(), right);
        UpdateCursorBoundsAndTextOffset();
    }

    void NativeTextfieldView::UpdateVerticalMargins()
    {
        int top, bottom;
        if(!textfield_->GetVerticalMargins(&top, &bottom))
        {
            return;
        }
        gfx::Insets inset = GetInsets();

        text_border_->SetInsets(top, inset.left(), bottom, inset.right());
        UpdateCursorBoundsAndTextOffset();
    }

    bool NativeTextfieldView::SetFocus()
    {
        return false;
    }

    View* NativeTextfieldView::GetView()
    {
        return this;
    }

    HWND NativeTextfieldView::GetTestingHandle() const
    {
        NOTREACHED();
        return NULL;
    }

    bool NativeTextfieldView::IsIMEComposing() const
    {
        return false;
    }

    void NativeTextfieldView::GetSelectedRange(Range* range) const
    {
        model_->GetSelectedRange(range);
    }

    void NativeTextfieldView::SelectRange(const Range& range)
    {
        model_->SelectRange(range);
        UpdateCursorBoundsAndTextOffset();
        SchedulePaint();
    }

    size_t NativeTextfieldView::GetCursorPosition() const
    {
        return model_->cursor_pos();
    }

    bool NativeTextfieldView::HandleKeyPressed(const KeyEvent& e)
    {
        TextfieldController* controller = textfield_->GetController();
        bool handled = false;
        if(controller)
        {
            handled = controller->HandleKeyEvent(textfield_, e);
        }
        return handled || HandleKeyEvent(e);
    }

    bool NativeTextfieldView::HandleKeyReleased(const KeyEvent& e)
    {
        return true;
    }

    void NativeTextfieldView::HandleFocus()
    {
        is_cursor_visible_ = true;
        SchedulePaint();
        // Start blinking cursor.
        MessageLoop::current()->PostDelayedTask(
            cursor_timer_.NewRunnableMethod(&NativeTextfieldView::UpdateCursor),
            kCursorVisibleTimeMs);
    }

    void NativeTextfieldView::HandleBlur()
    {
        // Stop blinking cursor.
        cursor_timer_.RevokeAll();
        if(is_cursor_visible_)
        {
            is_cursor_visible_ = false;
            RepaintCursor();
        }
    }

    /////////////////////////////////////////////////////////////////
    // NativeTextfieldView, SimpleMenuModel::Delegate overrides:

    bool NativeTextfieldView::IsCommandIdChecked(int command_id) const
    {
        return true;
    }

    bool NativeTextfieldView::IsCommandIdEnabled(int command_id) const
    {
        bool editable = !textfield_->read_only();
        string16 result;
        switch(command_id)
        {
        case IDS_APP_CUT:
            return editable && model_->HasSelection();
        case IDS_APP_COPY:
            return model_->HasSelection();
        case IDS_APP_PASTE:
            ViewDelegate::view_delegate->GetClipboard()->ReadText(
                Clipboard::BUFFER_STANDARD, &result);
            return editable && !result.empty();
        case IDS_APP_DELETE:
            return editable && model_->HasSelection();
        case IDS_APP_SELECT_ALL:
            return true;
        default:
            NOTREACHED();
            return false;
        }
    }

    bool NativeTextfieldView::GetAcceleratorForCommandId(int command_id,
        MenuAccelerator* accelerator)
    {
        return false;
    }

    void NativeTextfieldView::ExecuteCommand(int command_id)
    {
        bool text_changed = false;
        bool editable = !textfield_->read_only();
        switch(command_id)
        {
        case IDS_APP_CUT:
            if(editable)
            {
                text_changed = model_->Cut();
            }
            break;
        case IDS_APP_COPY:
            model_->Copy();
            break;
        case IDS_APP_PASTE:
            if(editable)
            {
                text_changed = model_->Paste();
            }
            break;
        case IDS_APP_DELETE:
            if(editable)
            {
                text_changed = model_->Delete();
            }
            break;
        case IDS_APP_SELECT_ALL:
            SelectAll();
            break;
        default:
            NOTREACHED() << "unknown command: " << command_id;
            break;
        }

        // The cursor must have changed if text changed during cut/paste/delete.
        UpdateAfterChange(text_changed, text_changed);
    }

    // static
    bool NativeTextfieldView::IsTextfieldViewEnabled()
    {
        return textfield_view_enabled ||
            base::CommandLine::ForCurrentProcess()->HasSwitch(
            kEnableViewsBasedTextfieldSwitch);
    }

    // static
    void NativeTextfieldView::SetEnableTextfieldView(bool enabled)
    {
        textfield_view_enabled = enabled;
    }

    void NativeTextfieldView::OnBoundsChanged(const gfx::Rect& previous_bounds)
    {
        UpdateCursorBoundsAndTextOffset();
    }


    ///////////////////////////////////////////////////////////////////////////////
    // NativeTextfieldView private:

    const gfx::Font& NativeTextfieldView::GetFont() const
    {
        return textfield_->font();
    }

    SkColor NativeTextfieldView::GetTextColor() const
    {
        return textfield_->text_color();
    }

    void NativeTextfieldView::UpdateCursor()
    {
        is_cursor_visible_ = !is_cursor_visible_;
        RepaintCursor();
        MessageLoop::current()->PostDelayedTask(
            cursor_timer_.NewRunnableMethod(&NativeTextfieldView::UpdateCursor),
            is_cursor_visible_?kCursorVisibleTimeMs:kCursorInvisibleTimeMs);
    }

    void NativeTextfieldView::RepaintCursor()
    {
        gfx::Rect r = cursor_bounds_;
        r.Inset(-1, -1, -1, -1);
        SchedulePaintInRect(r);
    }

    void NativeTextfieldView::UpdateCursorBoundsAndTextOffset()
    {
        if(bounds().IsEmpty())
        {
            return;
        }

        gfx::Insets insets = GetInsets();

        int width = bounds().width() - insets.width();

        // TODO(oshima): bidi
        const gfx::Font& font = GetFont();
        int full_width = font.GetStringWidth(model_->GetVisibleText());
        cursor_bounds_ = model_->GetCursorBounds(font);
        cursor_bounds_.set_y(cursor_bounds_.y() + insets.top());

        int x_right = text_offset_ + cursor_bounds_.right();
        int x_left = text_offset_ + cursor_bounds_.x();

        if(full_width < width)
        {
            // Show all text whenever the text fits to the size.
            text_offset_ = 0;
        }
        else if(x_right > width)
        {
            // when the cursor overflows to the right
            text_offset_ = width - cursor_bounds_.right();
        }
        else if(x_left < 0)
        {
            // when the cursor overflows to the left
            text_offset_ = -cursor_bounds_.x();
        }
        else if(full_width>width && text_offset_+full_width<width)
        {
            // when the cursor moves within the textfield with the text
            // longer than the field.
            text_offset_ = width - full_width;
        }
        else
        {
            // move cursor freely.
        }
        // shift cursor bounds to fit insets.
        cursor_bounds_.set_x(cursor_bounds_.x() + text_offset_ + insets.left());
    }

    void NativeTextfieldView::PaintTextAndCursor(gfx::Canvas* canvas)
    {
        gfx::Insets insets = GetInsets();

        canvas->Save();
        canvas->ClipRectInt(insets.left(), insets.top(),
            width()-insets.width(), height()-insets.height());

        // TODO(oshima): bidi support
        // TODO(varunjain): re-implement this so only that dirty text is painted.
        TextfieldViewsModel::TextFragments fragments;
        model_->GetFragments(&fragments);
        int x_offset = text_offset_ + insets.left();
        int y = insets.top();
        int text_height = height() - insets.height();
        SkColor selection_color = textfield_->HasFocus() ? kFocusedSelectionColor :
            kUnfocusedSelectionColor;
        SkColor text_color = textfield_->read_only() ? kReadonlyTextColor :
            GetTextColor();

        for(TextfieldViewsModel::TextFragments::const_iterator iter=fragments.begin();
            iter!=fragments.end(); ++iter)
        {
            string16 text = model_->GetVisibleText((*iter).begin, (*iter).end);
            // TODO(oshima): This does not give the accurate position due to
            // kerning. Figure out how webkit does this with skia.
            int width = GetFont().GetStringWidth(text);

            if((*iter).selected)
            {
                canvas->FillRectInt(selection_color, x_offset, y, width, text_height);
                canvas->DrawStringInt(text, GetFont(), kSelectedTextColor,
                    x_offset, y, width, text_height);
            }
            else
            {
                canvas->DrawStringInt(text, GetFont(), text_color,
                    x_offset, y, width, text_height);
            }
            x_offset += width;
        }
        canvas->Restore();

        if(textfield_->IsEnabled() && is_cursor_visible_ &&
            !model_->HasSelection())
        {
            // Paint Cursor. Replace cursor is drawn as rectangle for now.
            canvas->DrawRectInt(kCursorColor,
                cursor_bounds_.x(),
                cursor_bounds_.y(),
                insert_?0:cursor_bounds_.width(),
                cursor_bounds_.height());
        }
    }

    bool NativeTextfieldView::HandleKeyEvent(const KeyEvent& key_event)
    {
        // TODO(oshima): handle IME.
        if(key_event.type() == ET_KEY_PRESSED)
        {
            KeyboardCode key_code = key_event.key_code();
            // TODO(oshima): shift-tab does not work. Figure out why and fix.
            if(key_code == VKEY_TAB)
            {
                return false;
            }
            bool editable = !textfield_->read_only();
            bool selection = key_event.IsShiftDown();
            bool control = key_event.IsControlDown();
            bool text_changed = false;
            bool cursor_changed = false;
            switch(key_code)
            {
            case VKEY_A:
                if(control)
                {
                    model_->SelectAll();
                    cursor_changed = true;
                }
                break;
            case VKEY_X:
                if(control && editable)
                {
                    cursor_changed = text_changed = model_->Cut();
                }
                break;
            case VKEY_C:
                if(control)
                {
                    model_->Copy();
                }
                break;
            case VKEY_V:
                if(control && editable)
                {
                    cursor_changed = text_changed = model_->Paste();
                }
                break;
            case VKEY_RIGHT:
                control ? model_->MoveCursorToNextWord(selection)
                    : model_->MoveCursorRight(selection);
                cursor_changed = true;
                break;
            case VKEY_LEFT:
                control ? model_->MoveCursorToPreviousWord(selection)
                    : model_->MoveCursorLeft(selection);
                cursor_changed = true;
                break;
            case VKEY_END:
                model_->MoveCursorToEnd(selection);
                cursor_changed = true;
                break;
            case VKEY_HOME:
                model_->MoveCursorToStart(selection);
                cursor_changed = true;
                break;
            case VKEY_BACK:
                if(!editable)
                {
                    break;
                }
                if(!model_->HasSelection())
                {
                    if(selection && control)
                    {
                        // If both shift and control are pressed, then erase upto the
                        // beginning of the buffer in ChromeOS. In windows, do nothing.
                        break;
                    }
                    else if(control)
                    {
                        // If only control is pressed, then erase the previous word.
                        model_->MoveCursorToPreviousWord(true);
                    }
                }
                text_changed = model_->Backspace();
                cursor_changed = true;
                break;
            case VKEY_DELETE:
                if(!editable)
                {
                    break;
                }
                if(!model_->HasSelection())
                {
                    if(selection && control)
                    {
                        // If both shift and control are pressed, then erase upto the
                        // end of the buffer in ChromeOS. In windows, do nothing.
                        break;
                    }
                    else if(control)
                    {
                        // If only control is pressed, then erase the next word.
                        model_->MoveCursorToNextWord(true);
                    }
                }
                cursor_changed = text_changed = model_->Delete();
                break;
            case VKEY_INSERT:
                insert_ = !insert_;
                cursor_changed = true;
                break;
            default:
                break;
            }
            char16 print_char = GetPrintableChar(key_event);
            if(!control && print_char && editable)
            {
                if(insert_)
                {
                    model_->Insert(print_char);
                }
                else
                {
                    model_->Replace(print_char);
                }
                text_changed = true;
            }

            UpdateAfterChange(text_changed, cursor_changed);
            return (text_changed || cursor_changed);
        }
        return false;
    }

    char16 NativeTextfieldView::GetPrintableChar(const KeyEvent& key_event)
    {
        // TODO(oshima): IME, i18n support.
        // This only works for UCS-2 characters.
        KeyboardCode key_code = key_event.key_code();
        bool shift = key_event.IsShiftDown();
        bool upper = shift ^ key_event.IsCapsLockDown();
        // TODO(oshima): We should have a utility function
        // under app to convert a KeyboardCode to a printable character,
        // probably in keyboard_code_conversion{.h, _x
        switch(key_code)
        {
        case VKEY_NUMPAD0:
            return '0';
        case VKEY_NUMPAD1:
            return '1';
        case VKEY_NUMPAD2:
            return '2';
        case VKEY_NUMPAD3:
            return '3';
        case VKEY_NUMPAD4:
            return '4';
        case VKEY_NUMPAD5:
            return '5';
        case VKEY_NUMPAD6:
            return '6';
        case VKEY_NUMPAD7:
            return '7';
        case VKEY_NUMPAD8:
            return '8';
        case VKEY_NUMPAD9:
            return '9';
        case VKEY_MULTIPLY:
            return '*';
        case VKEY_ADD:
            return '+';
        case VKEY_SUBTRACT:
            return '-';
        case VKEY_DECIMAL:
            return '.';
        case VKEY_DIVIDE:
            return '/';
        case VKEY_SPACE:
            return ' ';
        case VKEY_0:
            return shift ? ')' : '0';
        case VKEY_1:
            return shift ? '!' : '1';
        case VKEY_2:
            return shift ? '@' : '2';
        case VKEY_3:
            return shift ? '#' : '3';
        case VKEY_4:
            return shift ? '$' : '4';
        case VKEY_5:
            return shift ? '%' : '5';
        case VKEY_6:
            return shift ? '^' : '6';
        case VKEY_7:
            return shift ? '&' : '7';
        case VKEY_8:
            return shift ? '*' : '8';
        case VKEY_9:
            return shift ? '(' : '9';

        case VKEY_A:
        case VKEY_B:
        case VKEY_C:
        case VKEY_D:
        case VKEY_E:
        case VKEY_F:
        case VKEY_G:
        case VKEY_H:
        case VKEY_I:
        case VKEY_J:
        case VKEY_K:
        case VKEY_L:
        case VKEY_M:
        case VKEY_N:
        case VKEY_O:
        case VKEY_P:
        case VKEY_Q:
        case VKEY_R:
        case VKEY_S:
        case VKEY_T:
        case VKEY_U:
        case VKEY_V:
        case VKEY_W:
        case VKEY_X:
        case VKEY_Y:
        case VKEY_Z:
            return (upper ? 'A' : 'a') + (key_code - VKEY_A);
        case VKEY_OEM_1:
            return shift ? ':' : ';';
        case VKEY_OEM_PLUS:
            return shift ? '+' : '=';
        case VKEY_OEM_COMMA:
            return shift ? '<' : ',';
        case VKEY_OEM_MINUS:
            return shift ? '_' : '-';
        case VKEY_OEM_PERIOD:
            return shift ? '>' : '.';
        case VKEY_OEM_2:
            return shift ? '?' : '/';
        case VKEY_OEM_3:
            return shift ? '~' : '`';
        case VKEY_OEM_4:
            return shift ? '}' : ']';
        case VKEY_OEM_5:
            return shift ? '|' : '\\';
        case VKEY_OEM_6:
            return shift ? '{' : '[';
        case VKEY_OEM_7:
            return shift ? '"' : '\'';
        default:
            return 0;
        }
    }

    size_t NativeTextfieldView::FindCursorPosition(const gfx::Point& point) const
    {
        // TODO(oshima): BIDI/i18n support.
        gfx::Font font = GetFont();
        gfx::Insets insets = GetInsets();
        string16 text = model_->GetVisibleText();
        int left = 0;
        int left_pos = 0;
        int right = font.GetStringWidth(text);
        int right_pos = text.length();

        int x = point.x() - insets.left() - text_offset_;
        if(x <= left) return left_pos;
        if(x >= right) return right_pos;
        // binary searching the cursor position.
        // TODO(oshima): use the center of character instead of edge.
        // Binary search may not work for language like arabic.
        while(std::abs(static_cast<long>(right_pos - left_pos) > 1))
        {
            int pivot_pos = left_pos + (right_pos-left_pos)/2;
            int pivot = font.GetStringWidth(text.substr(0, pivot_pos));
            if(pivot < x)
            {
                left = pivot;
                left_pos = pivot_pos;
            }
            else if(pivot == x)
            {
                return pivot_pos;
            }
            else
            {
                right = pivot;
                right_pos = pivot_pos;
            }
        }
        return left_pos;
    }

    bool NativeTextfieldView::HandleMousePressed(const MouseEvent& e)
    {
        textfield_->RequestFocus();
        base::TimeDelta time_delta = e.time_stamp() - last_mouse_press_time_;
        gfx::Point location_delta = e.location().Subtract(last_mouse_press_location_);
        last_mouse_press_time_ = e.time_stamp();
        last_mouse_press_location_ = e.location();
        if(e.IsLeftMouseButton())
        {
            if(!ExceededDragThreshold(location_delta.x(), location_delta.y())
                && time_delta.InMilliseconds()<=GetDoubleClickInterval())
            {
                // Multiple mouse press detected. Check for double or triple.
                switch(click_state_)
                {
                case TRACKING_DOUBLE_CLICK:
                    click_state_ = TRACKING_TRIPLE_CLICK;
                    model_->SelectWord();
                    return true;
                case TRACKING_TRIPLE_CLICK:
                    click_state_ = NONE;
                    model_->SelectAll();
                    return true;
                case NONE:
                    click_state_ = TRACKING_DOUBLE_CLICK;
                    SetCursorForMouseClick(e);
                    return true;
                }
            }
            else
            {
                // Single mouse press.
                click_state_ = TRACKING_DOUBLE_CLICK;
                SetCursorForMouseClick(e);
                return true;
            }
        }
        return false;
    }

    void NativeTextfieldView::SetCursorForMouseClick(const MouseEvent& e)
    {
        size_t pos = FindCursorPosition(e.location());
        if(model_->MoveCursorTo(pos, false))
        {
            UpdateCursorBoundsAndTextOffset();
        }
    }

    void NativeTextfieldView::PropagateTextChange()
    {
        textfield_->SyncText();
        TextfieldController* controller = textfield_->GetController();
        if(controller)
        {
            controller->ContentsChanged(textfield_, GetText());
        }
    }

    void NativeTextfieldView::UpdateAfterChange(bool text_changed,
        bool cursor_changed)
    {
        if(text_changed)
        {
            PropagateTextChange();
        }
        if(cursor_changed)
        {
            is_cursor_visible_ = true;
            RepaintCursor();
        }
        if(text_changed || cursor_changed)
        {
            UpdateCursorBoundsAndTextOffset();
            SchedulePaint();
        }
    }

    void NativeTextfieldView::InitContextMenuIfRequired()
    {
        if(context_menu_menu_.get())
        {
            return;
        }
        context_menu_contents_.reset(new SimpleMenuModel(this));
        context_menu_contents_->AddItemWithStringId(IDS_APP_CUT, IDS_APP_CUT);
        context_menu_contents_->AddItemWithStringId(IDS_APP_COPY, IDS_APP_COPY);
        context_menu_contents_->AddItemWithStringId(IDS_APP_PASTE, IDS_APP_PASTE);
        context_menu_contents_->AddItemWithStringId(IDS_APP_DELETE, IDS_APP_DELETE);
        context_menu_contents_->AddSeparator();
        context_menu_contents_->AddItemWithStringId(IDS_APP_SELECT_ALL,
            IDS_APP_SELECT_ALL);
        context_menu_menu_.reset(new Menu2(context_menu_contents_.get()));
    }

    ///////////////////////////////////////////////////////////////////////////////
    //
    // TextifieldBorder
    //
    ///////////////////////////////////////////////////////////////////////////////

    NativeTextfieldView::TextfieldBorder::TextfieldBorder()
        : has_focus_(false),
        insets_(4, 4, 4, 4) {}

    void NativeTextfieldView::TextfieldBorder::Paint(
        const View& view, gfx::Canvas* canvas) const
    {
        SkRect rect;
        rect.set(SkIntToScalar(0), SkIntToScalar(0),
            SkIntToScalar(view.width()), SkIntToScalar(view.height()));
        SkScalar corners[8] =
        {
            // top-left
            SkIntToScalar(insets_.left()),
            SkIntToScalar(insets_.top()),
            // top-right
            SkIntToScalar(insets_.right()),
            SkIntToScalar(insets_.top()),
            // bottom-right
            SkIntToScalar(insets_.right()),
            SkIntToScalar(insets_.bottom()),
            // bottom-left
            SkIntToScalar(insets_.left()),
            SkIntToScalar(insets_.bottom()),
        };
        SkPath path;
        path.addRoundRect(rect, corners);
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setFlags(SkPaint::kAntiAlias_Flag);
        // TODO(oshima): Copy what WebKit does for focused border.
        paint.setColor(has_focus_ ? kFocusedBorderColor : kDefaultBorderColor);
        paint.setStrokeWidth(SkIntToScalar(has_focus_ ? 2 : 1));

        canvas->AsCanvasSkia()->drawPath(path, paint);
    }

    void NativeTextfieldView::TextfieldBorder::GetInsets(gfx::Insets* insets) const
    {
        *insets = insets_;
    }

    void NativeTextfieldView::TextfieldBorder::SetInsets(int top,
        int left,
        int bottom,
        int right)
    {
        insets_.Set(top, left, bottom, right);
    }

} //namespace view