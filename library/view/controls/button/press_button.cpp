
#include "press_button.h"

#include "base/message_loop.h"

#include "ui_base/accessibility/accessible_view_state.h"
#include "ui_base/l10n/l10n_util.h"
#include "ui_base/resource/app_res_ids.h"

namespace view
{

    // static
    const char PressButton::kViewClassName[] = "view/ButtonPress";


    ////////////////////////////////////////////////////////////////////////////////
    //
    // PressButton - constructors, destructors, initialization, cleanup
    //
    ////////////////////////////////////////////////////////////////////////////////

    PressButton::PressButton(ButtonListener* listener)
        : ImageButton(listener),
		isPressed_(false)
	{}

    PressButton::~PressButton() {}


	void PressButton::setIsPressed(bool isPressed)
	{
		isPressed_ = isPressed;
		SetState(isPressed_ ? BS_PUSHED : BS_NORMAL);
	}
    ////////////////////////////////////////////////////////////////////////////////
    //
    // PressButton - Events
    //
    ////////////////////////////////////////////////////////////////////////////////

    bool PressButton::OnMousePressed(const MouseEvent& event)
    {
		isPressed_ = !isPressed_;
		SetState(isPressed_ ? BS_PUSHED : BS_NORMAL);
        return ImageButton::OnMousePressed(event);
    }

    bool PressButton::OnMouseDragged(const MouseEvent& event)
    {
		return false;
    }

    void PressButton::OnMouseReleased(const MouseEvent& event)
    {
		SetState(isPressed_ ? BS_PUSHED : BS_NORMAL);
    }

    std::string PressButton::GetClassName() const
    {
        return kViewClassName;
    }

	 void PressButton::OnMouseEntered(const MouseEvent& event)
	{
		if (isPressed_)
			return ;
		ImageButton::OnMouseEntered(event);
	}
	void PressButton::OnMouseExited(const MouseEvent& event)
    {
        // Starting a drag results in a MouseExited, we need to ignore it.
        // A right click release triggers an exit event. We want to
        // remain in a PUSHED state until the drop down menu closes.
        if(state_==BS_DISABLED)
			return;
		SetState(isPressed_ ? BS_PUSHED : BS_NORMAL);
    }

} //namespace view