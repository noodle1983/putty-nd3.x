
#ifndef __demo_silverlight_h__
#define __demo_silverlight_h__

#pragma once

#include "view/controls/button/button.h"

#include "demo_base.h"

namespace view
{
    class NativeTextButton;
    class SilverlightView;
}

class DemoSilverlight : public DemoBase, public view::ButtonListener
{
public:
    explicit DemoSilverlight(DemoMain* main);
    virtual ~DemoSilverlight();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from view::ButtonListener:
    virtual void ButtonPressed(view::Button* sender,
        const view::Event& event);

private:
    view::SilverlightView* silverlight_view1_;
    view::SilverlightView* silverlight_view2_;
    view::NativeTextButton* button_;

    DISALLOW_COPY_AND_ASSIGN(DemoSilverlight);
};

#endif //__demo_silverlight_h__