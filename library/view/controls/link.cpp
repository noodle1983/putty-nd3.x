
#include "link.h"

#include "gfx/color_utils.h"

#include "../accessibility/accessible_view_state.h"
#include "../widget/widget.h"

namespace
{

    void GetColors(const SkColor* background_color, // NULL means "use default"
        SkColor* highlighted_color,
        SkColor* disabled_color,
        SkColor* normal_color)
    {
        static SkColor kHighlightedColor, kDisabledColor, kNormalColor;
        static bool initialized = false;
        if(!initialized)
        {
            kHighlightedColor = gfx::GetReadableColor(
                SkColorSetRGB(200, 0, 0), gfx::GetSysSkColor(COLOR_WINDOW));
            kDisabledColor = gfx::GetSysSkColor(COLOR_WINDOWTEXT);
            kNormalColor = gfx::GetSysSkColor(COLOR_HOTLIGHT);

            initialized = true;
        }

        if(background_color)
        {
            *highlighted_color = gfx::GetReadableColor(kHighlightedColor,
                *background_color);
            *disabled_color = gfx::GetReadableColor(kDisabledColor,
                *background_color);
            *normal_color = gfx::GetReadableColor(kNormalColor,
                *background_color);
        }
        else
        {
            *highlighted_color = kHighlightedColor;
            *disabled_color = kDisabledColor;
            *normal_color = kNormalColor;
        }
    }

}

namespace view
{

    static HCURSOR g_hand_cursor = NULL;

    const char Link::kViewClassName[] = "view/Link";

    Link::Link() : Label(L""), controller_(NULL), highlighted_(false)
    {
        Init();
        SetFocusable(true);
    }

    Link::Link(const std::wstring& title) : Label(title),
        controller_(NULL), highlighted_(false)
    {
        Init();
        SetFocusable(true);
    }

    void Link::Init()
    {
        GetColors(NULL, &highlighted_color_, &disabled_color_,
            &normal_color_);
        SetColor(normal_color_);
        ValidateStyle();
    }

    Link::~Link() {}

    void Link::SetController(LinkController* controller)
    {
        controller_ = controller;
    }

    const LinkController* Link::GetController()
    {
        return controller_;
    }

    bool Link::OnMousePressed(const MouseEvent& event)
    {
        if(!enabled_ || (!event.IsLeftMouseButton() && !event.IsMiddleMouseButton()))
        {
            return false;
        }
        SetHighlighted(true);
        return true;
    }

    bool Link::OnMouseDragged(const MouseEvent& event)
    {
        SetHighlighted(enabled_ &&
            (event.IsLeftMouseButton() || event.IsMiddleMouseButton()) &&
            HitTest(event.location()));
        return true;
    }

    void Link::OnMouseReleased(const MouseEvent& event)
    {
        // Change the highlight first just in case this instance is deleted
        // while calling the controller
        OnMouseCaptureLost();
        if(enabled_ && (event.IsLeftMouseButton() || event.IsMiddleMouseButton()) &&
            HitTest(event.location()))
        {
            // Focus the link on click.
            RequestFocus();

            if(controller_)
            {
                controller_->LinkActivated(this, event.flags());
            }
        }
    }

    void Link::OnMouseCaptureLost()
    {
        SetHighlighted(false);
    }

    bool Link::OnKeyPressed(const KeyEvent& event)
    {
        bool activate = ((event.key_code()==VKEY_SPACE) ||
            (event.key_code()==VKEY_RETURN));
        if(!activate)
        {
            return false;
        }

        SetHighlighted(false);

        // Focus the link on key pressed.
        RequestFocus();

        if(controller_)
        {
            controller_->LinkActivated(this, event.flags());
        }

        return true;
    }

    bool Link::SkipDefaultKeyEventProcessing(const KeyEvent& event)
    {
        // Make sure we don't process space or enter as accelerators.
        return (event.key_code()==VKEY_SPACE) || (event.key_code()==VKEY_RETURN);
    }

    void Link::GetAccessibleState(AccessibleViewState* state)
    {
        Label::GetAccessibleState(state);
        state->role = AccessibilityTypes::ROLE_LINK;
    }

    void Link::SetFont(const gfx::Font& font)
    {
        Label::SetFont(font);
        ValidateStyle();
    }

    void Link::SetEnabled(bool f)
    {
        if(f != enabled_)
        {
            enabled_ = f;
            ValidateStyle();
            SchedulePaint();
        }
    }

    bool Link::OnSetCursor(const gfx::Point& p)
    {
        if(!enabled_)
        {
            return false;
        }
        if(!g_hand_cursor)
        {
            g_hand_cursor = LoadCursor(NULL, IDC_HAND);
        }
        GetWidget()->SetCursor(g_hand_cursor);
        return true;
    }

    std::string Link::GetClassName() const
    {
        return kViewClassName;
    }

    void Link::SetHighlightedColor(const SkColor& color)
    {
        highlighted_color_ = color;
        ValidateStyle();
    }

    void Link::SetDisabledColor(const SkColor& color)
    {
        disabled_color_ = color;
        ValidateStyle();
    }

    void Link::SetNormalColor(const SkColor& color)
    {
        normal_color_ = color;
        ValidateStyle();
    }

    void Link::MakeReadableOverBackgroundColor(const SkColor& color)
    {
        GetColors(&color, &highlighted_color_, &disabled_color_,
            &normal_color_);
        ValidateStyle();
    }

    void Link::SetHighlighted(bool f)
    {
        if(f != highlighted_)
        {
            highlighted_ = f;
            ValidateStyle();
            SchedulePaint();
        }
    }

    void Link::ValidateStyle()
    {
        if(enabled_)
        {
            if(!(font().GetStyle() & gfx::Font::UNDERLINED))
            {
                Label::SetFont(font().DeriveFont(0,
                    font().GetStyle()|gfx::Font::UNDERLINED));
            }
            Label::SetColor(highlighted_ ? highlighted_color_ : normal_color_);
        }
        else
        {
            if(font().GetStyle() & gfx::Font::UNDERLINED)
            {
                Label::SetFont(font().DeriveFont(0,
                    font().GetStyle()&~gfx::Font::UNDERLINED));
            }
            Label::SetColor(disabled_color_);
        }
    }

} //namespace view