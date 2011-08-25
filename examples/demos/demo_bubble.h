
#ifndef __demo_bubble_h__
#define __demo_bubble_h__

#pragma once

#include "view/controls/button/button.h"

#include "demo_base.h"

namespace view
{
    class TextButton;
}

class Bubble;

class DemoBubble : public DemoBase, public view::ButtonListener
{
public:
    explicit DemoBubble(DemoMain* main);
    virtual ~DemoBubble();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from view::ButtonListener:
    virtual void ButtonPressed(view::Button* sender,
        const view::Event& event);

private:
    view::TextButton* button1_;
    view::TextButton* button2_;
    view::TextButton* button3_;
    view::TextButton* button4_;
    view::TextButton* button5_;

    DISALLOW_COPY_AND_ASSIGN(DemoBubble);
};

#endif //__demo_bubble_h__