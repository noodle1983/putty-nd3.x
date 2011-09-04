
#include "radio_button.h"

#include "base/logging.h"

#include "ui_base/accessibility/accessible_view_state.h"

#include "view/widget/widget.h"

namespace view
{

    // static
    const char RadioButton::kViewClassName[] = "view/RadioButton";

    ////////////////////////////////////////////////////////////////////////////////
    //
    // RadioButton
    //
    ////////////////////////////////////////////////////////////////////////////////

    RadioButton::RadioButton(const std::wstring& label, int group)
        : Checkbox(label)
    {
        SetGroup(group);
        set_focusable(true);
    }

    RadioButton::~RadioButton() {}

    void RadioButton::SetChecked(bool checked)
    {
        if(checked == RadioButton::checked())
        {
            return;
        }
        if(checked)
        {
            // We can't just get the root view here because sometimes the radio
            // button isn't attached to a root view (e.g., if it's part of a tab page
            // that is currently not active).
            View* container = parent();
            while(container && container->parent())
            {
                container = container->parent();
            }
            if(container)
            {
                Views other;
                container->GetViewsInGroup(GetGroup(), &other);
                for(Views::iterator i(other.begin()); i!=other.end(); ++i)
                {
                    if(*i != this)
                    {
                        if((*i)->GetClassName() != kViewClassName)
                        {
                            NOTREACHED() << "radio-button has same group as other non "
                                "radio-button views.";
                            continue;
                        }
                        RadioButton* peer = static_cast<RadioButton*>(*i);
                        peer->SetChecked(false);
                    }
                }
            }
        }
        Checkbox::SetChecked(checked);
    }

    std::string RadioButton::GetClassName() const
    {
        return kViewClassName;
    }

    void RadioButton::GetAccessibleState(ui::AccessibleViewState* state)
    {
        Checkbox::GetAccessibleState(state);
        state->role = ui::AccessibilityTypes::ROLE_RADIOBUTTON;
    }

    View* RadioButton::GetSelectedViewForGroup(int group)
    {
        Views views;
        GetWidget()->GetRootView()->GetViewsInGroup(group, &views);
        if(views.empty())
        {
            return NULL;
        }

        for(Views::const_iterator i(views.begin()); i!=views.end(); ++i)
        {
            // REVIEW: why don't we check the runtime type like is done above?
            RadioButton* radio_button = static_cast<RadioButton*>(*i);
            if(radio_button->checked())
            {
                return radio_button;
            }
        }
        return NULL;
    }

    bool RadioButton::IsGroupFocusTraversable() const
    {
        // When focusing a radio button with tab/shift+tab, only the selected button
        // from the group should be focused.
        return false;
    }

    void RadioButton::OnFocus()
    {
        Checkbox::OnFocus();
        SetChecked(true);
        MouseEvent event(ui::ET_MOUSE_PRESSED, 0, 0, 0);
        TextButtonBase::NotifyClick(event);
    }

    void RadioButton::NotifyClick(const Event& event)
    {
        // Set the checked state to true only if we are unchecked, since we can't
        // be toggled on and off like a checkbox.
        if(!checked())
        {
            SetChecked(true);
        }
        RequestFocus();
        TextButtonBase::NotifyClick(event);
    }

    gfx::NativeTheme::Part RadioButton::GetThemePart() const
    {
        return gfx::NativeTheme::kRadio;
    }

} //namespace view