
#ifndef __view_checkbox_h__
#define __view_checkbox_h__

#pragma once

#include <string>

#include "text_button.h"

namespace view
{

    // A native themed class representing a checkbox.  This class does not use
    // platform specific objects to replicate the native platforms looks and feel.
    class Checkbox : public TextButtonBase
    {
    public:
        // The button's class name.
        static const char kViewClassName[];

        explicit Checkbox(const std::wstring& label);
        virtual ~Checkbox();

        // Sets a listener for this checkbox. Checkboxes aren't required to have them
        // since their state can be read independently of them being toggled.
        void set_listener(ButtonListener* listener) { listener_ = listener; }

        // Sets/Gets whether or not the checkbox is checked.
        virtual void SetChecked(bool checked);
        bool checked() const { return checked_; }

    protected:
        // Overridden from View:
        virtual gfx::Size GetPreferredSize();
        virtual std::string GetClassName() const;
        virtual void GetAccessibleState(ui::AccessibleViewState* state);
        virtual void OnPaintFocusBorder(gfx::Canvas* canvas);

    private:
        // Overridden from Button:
        virtual void NotifyClick(const Event& event);

        // Overridden from TextButtonBase:
        virtual gfx::NativeTheme::Part GetThemePart() const;
        virtual gfx::Rect GetThemePaintRect() const;
        virtual void GetExtraParams(
            gfx::NativeTheme::ExtraParams* params) const;
        virtual gfx::Rect GetTextBounds() const;

        // True if the checkbox is checked.
        bool checked_;

        DISALLOW_COPY_AND_ASSIGN(Checkbox);
    };

} //namespace view

#endif //__view_checkbox_h__