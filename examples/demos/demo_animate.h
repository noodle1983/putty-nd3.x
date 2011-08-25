
#ifndef __demo_animate_h__
#define __demo_animate_h__

#pragma once

#include "view/controls/button/button.h"

#include "demo_base.h"

namespace view
{
    class NativeTextButton;
}

class DemoAnimate : public DemoBase, public view::ButtonListener
{
public:
    explicit DemoAnimate(DemoMain* main);
    virtual ~DemoAnimate();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from view::ButtonListener:
    virtual void ButtonPressed(view::Button* sender,
        const view::Event& event);

private:
    view::View* apps_container_;
    view::NativeTextButton* button_animate_;

    DISALLOW_COPY_AND_ASSIGN(DemoAnimate);
};

#endif //__demo_animate_h__