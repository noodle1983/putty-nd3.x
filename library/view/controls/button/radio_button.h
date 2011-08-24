
#ifndef __view_radio_button_h__
#define __view_radio_button_h__

#pragma once

#include "checkbox.h"

namespace view
{

    // A native themed class representing a radio button.  This class does not use
    // platform specific objects to replicate the native platforms looks and feel.
    class RadioButton : public Checkbox
    {
    public:
        // The button's class name.
        static const char kViewClassName[];

        RadioButton(const std::wstring& label, int group);
        virtual ~RadioButton();

        // Overridden from View:
        virtual std::string GetClassName() const;
        virtual void GetAccessibleState(ui::AccessibleViewState* state);
        virtual View* GetSelectedViewForGroup(int group);
        virtual bool IsGroupFocusTraversable() const;
        virtual void OnFocus();

        // Overridden from Button:
        virtual void NotifyClick(const Event& event);

        // Overridden from TextButtonBase:
        virtual gfx::NativeTheme::Part GetThemePart() const;

        // Overridden from Checkbox:
        virtual void SetChecked(bool checked);

        DISALLOW_COPY_AND_ASSIGN(RadioButton);
    };

} //namespace view

#endif //__view_radio_button_h__