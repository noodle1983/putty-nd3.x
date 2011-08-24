
#include "checkbox.h"

#include "base/logging.h"

#include "ui_gfx/canvas.h"

#include "ui_base/accessibility/accessible_view_state.h"

namespace view
{

    // static
    const char Checkbox::kViewClassName[] = "view/Checkbox";

    static const int kCheckboxLabelSpacing = 4;

    ////////////////////////////////////////////////////////////////////////////////
    // Checkbox, public:

    Checkbox::Checkbox(const std::wstring& label)
        : TextButtonBase(NULL, label), checked_(false)
    {
        set_border(new TextButtonNativeThemeBorder(this));
        set_focusable(true);
    }

    Checkbox::~Checkbox() {}

    void Checkbox::SetChecked(bool checked)
    {
        checked_ = checked;
        SchedulePaint();
    }

    gfx::Size Checkbox::GetPreferredSize()
    {
        gfx::Size prefsize(TextButtonBase::GetPreferredSize());
        gfx::NativeTheme::ExtraParams extra;
        gfx::NativeTheme::State state = GetThemeState(&extra);
        gfx::Size size = gfx::NativeTheme::instance()->GetPartSize(GetThemePart(),
            state, extra);
        prefsize.Enlarge(size.width(), 0);
        prefsize.set_height(std::max(prefsize.height(), size.height()));

        if(max_width_ > 0)
        {
            prefsize.set_width(std::min(max_width_, prefsize.width()));
        }

        return prefsize;
    }

    std::string Checkbox::GetClassName() const
    {
        return kViewClassName;
    }

    void Checkbox::GetAccessibleState(ui::AccessibleViewState* state)
    {
        TextButtonBase::GetAccessibleState(state);
        state->role = ui::AccessibilityTypes::ROLE_CHECKBUTTON;
        state->state = checked() ? ui::AccessibilityTypes::STATE_CHECKED : 0;
    }

    void Checkbox::OnPaintFocusBorder(gfx::Canvas* canvas)
    {
        if(HasFocus() && (IsFocusable() || IsAccessibilityFocusableInRootView()))
        {
            gfx::Rect bounds(GetTextBounds());
            // Increate the bounding box by one on each side so that that focus border
            // does not draw on top of the letters.
            bounds.Inset(-1, -1, -1, -1);
            canvas->DrawFocusRect(bounds.x(), bounds.y(),
                bounds.width(), bounds.height());
        }
    }

    void Checkbox::NotifyClick(const Event& event)
    {
        SetChecked(!checked());
        RequestFocus();
        TextButtonBase::NotifyClick(event);
    }

    gfx::NativeTheme::Part Checkbox::GetThemePart() const
    {
        return gfx::NativeTheme::kCheckbox;
    }

    gfx::Rect Checkbox::GetThemePaintRect() const
    {
        gfx::NativeTheme::ExtraParams extra;
        gfx::NativeTheme::State state = GetThemeState(&extra);
        gfx::Size size(gfx::NativeTheme::instance()->GetPartSize(GetThemePart(),
            state, extra));
        gfx::Insets insets = GetInsets();
        gfx::Rect rect(insets.left(), 0, size.width(), height());
        rect.set_x(GetMirroredXForRect(rect));
        return rect;
    }

    void Checkbox::GetExtraParams(gfx::NativeTheme::ExtraParams* params) const
    {
        TextButtonBase::GetExtraParams(params);
        params->button.checked = checked_;
    }

    gfx::Rect Checkbox::GetTextBounds() const
    {
        gfx::Rect bounds(TextButtonBase::GetTextBounds());
        gfx::NativeTheme::ExtraParams extra;
        gfx::NativeTheme::State state = GetThemeState(&extra);
        gfx::Size size(gfx::NativeTheme::instance()->GetPartSize(GetThemePart(),
            state, extra));
        bounds.Offset(size.width() + kCheckboxLabelSpacing, 0);
        return bounds;
    }

} //namespace view