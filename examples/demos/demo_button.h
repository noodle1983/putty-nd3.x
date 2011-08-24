
#ifndef __demo_button_h__
#define __demo_button_h__

#pragma once

#include "view/controls/button/button.h"

#include "demo_base.h"

namespace view
{
    class Checkbox;
    class NativeCheckbox;
    class NativeRadioButton;
    class RadioButton;
}

class DemoButton : public DemoBase, public view::ButtonListener
{
public:
    explicit DemoButton(DemoMain* main);
    virtual ~DemoButton();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from view::ButtonListener:
    virtual void ButtonPressed(view::Button* sender,
        const view::Event& event);

private:
    view::Checkbox* checkbox_;

    view::RadioButton* radio1_;
    view::RadioButton* radio2_;
    view::RadioButton* radio3_;

    DISALLOW_COPY_AND_ASSIGN(DemoButton);
};

#endif //__demo_button_h__