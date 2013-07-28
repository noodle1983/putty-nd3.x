#ifndef __view_press_button_h__
#define __view_press_button_h__

#pragma once

#include "image_button.h"

namespace view
{

    // An image button.

    // Note that this type of button is not focusable by default and will not be
    // part of the focus chain.  Call SetFocusable(true) to make it part of the
    // focus chain.
    class PressButton : public ImageButton
    {
    public:
        // The button's class name.
        static const char kViewClassName[];

        explicit  PressButton(ButtonListener* listener);
        virtual ~PressButton();

		void setIsPressed(bool isPressed);
		bool isPressed(){return isPressed_;}
        // Overridden from view::View
        virtual bool OnMousePressed(const MouseEvent& event);
        virtual bool OnMouseDragged(const MouseEvent& event);
        virtual void OnMouseReleased(const MouseEvent& event);
        virtual std::string GetClassName() const;
		virtual void OnMouseCaptureLost() {}
		virtual void OnMouseEntered(const MouseEvent& event);
        virtual void OnMouseExited(const MouseEvent& event);
		virtual void OnMouseMoved(const MouseEvent& event){}
	private:
		bool isPressed_;
	};
}
#endif 

