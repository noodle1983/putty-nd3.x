
#ifndef __view_custom_button_h__
#define __view_custom_button_h__

#pragma once

#include "animation/animation_delegate.h"

#include "button.h"

class ThrobAnimation;

namespace view
{

    // A button with custom rendering. The common base class of ImageButton and
    // TextButton.
    // Note that this type of button is not focusable by default and will not be
    // part of the focus chain.  Call SetFocusable(true) to make it part of the
    // focus chain.
    class CustomButton : public Button, public AnimationDelegate
    {
    public:
        // The menu button's class name.
        static const char kViewClassName[];

        virtual ~CustomButton();

        // Possible states
        enum ButtonState
        {
            BS_NORMAL = 0,
            BS_HOT,
            BS_PUSHED,
            BS_DISABLED,
            BS_COUNT
        };

        // Get/sets the current display state of the button.
        ButtonState state() const { return state_; }
        void SetState(ButtonState state);

        // Starts throbbing. See HoverAnimation for a description of cycles_til_stop.
        void StartThrobbing(int cycles_til_stop);

        // Stops throbbing immediately.
        void StopThrobbing();

        // Set how long the hover animation will last for.
        void SetAnimationDuration(int duration);

        void set_triggerable_event_flags(int triggerable_event_flags)
        {
            triggerable_event_flags_ = triggerable_event_flags;
        }
        int triggerable_event_flags() const { return triggerable_event_flags_; }

        // Sets whether |RequestFocus| should be invoked on a mouse press. The default
        // is true.
        void set_request_focus_on_press(bool value)
        {
            request_focus_on_press_ = value;
        }
        bool request_focus_on_press() const { return request_focus_on_press_; }

        // See description above field.
        void set_animate_on_state_change(bool value)
        {
            animate_on_state_change_ = value;
        }

        // Returns true if the mouse pointer is over this control.  Note that this
        // isn't the same as IsHotTracked() because the mouse may be over the control
        // when it's disabled.
        bool IsMouseHovered() const;

        // Overridden from View:
        virtual void SetHotTracked(bool flag);
        virtual bool IsHotTracked() const;
        virtual void SetEnabled(bool enabled);
        virtual bool IsEnabled() const;
        virtual std::string GetClassName() const;
        virtual bool OnMousePressed(const MouseEvent& event);
        virtual bool OnMouseDragged(const MouseEvent& event);
        virtual void OnMouseReleased(const MouseEvent& event);
        virtual void OnMouseCaptureLost();
        virtual void OnMouseEntered(const MouseEvent& event);
        virtual void OnMouseExited(const MouseEvent& event);
        virtual void OnMouseMoved(const MouseEvent& event);
        virtual bool OnKeyPressed(const KeyEvent& event);
        virtual bool OnKeyReleased(const KeyEvent& event);
        virtual bool AcceleratorPressed(const Accelerator& accelerator);
        virtual void ShowContextMenu(const gfx::Point& p,
            bool is_mouse_gesture);
        virtual void OnDragDone();
        virtual void GetAccessibleState(AccessibleViewState* state);

        // Overridden from AnimationDelegate:
        virtual void AnimationProgressed(const Animation* animation);

    protected:
        // Construct the Button with a Listener. See comment for Button's ctor.
        explicit CustomButton(ButtonListener* listener);

        // Returns true if the event is one that can trigger notifying the listener.
        // This implementation returns true if the left mouse button is down.
        virtual bool IsTriggerableEvent(const MouseEvent& e);

        // Returns true if the button should become pressed when the user
        // holds the mouse down over the button. For this implementation,
        // we simply return IsTriggerableEvent(event).
        virtual bool ShouldEnterPushedState(const MouseEvent& event);

        // Overridden from View:
        virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
        virtual bool IsFocusable() const;
        virtual void OnBlur();

        // The button state (defined in implementation)
        ButtonState state_;

        // Hover animation.
        scoped_ptr<ThrobAnimation> hover_animation_;

    private:
        // Should we animate when the state changes? Defaults to true.
        bool animate_on_state_change_;

        // Is the hover animation running because StartThrob was invoked?
        bool is_throbbing_;

        // Mouse event flags which can trigger button actions.
        int triggerable_event_flags_;

        // See description above setter.
        bool request_focus_on_press_;

        DISALLOW_COPY_AND_ASSIGN(CustomButton);
    };

} //namespace view

#endif //__view_custom_button_h__