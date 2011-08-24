
#ifndef __demo_html_h__
#define __demo_html_h__

#pragma once

#include "view/controls/button/button.h"

#include "demo_base.h"

namespace view
{
    class HtmlView;
    class NativeTextButton;
    class Textfield;
}

class DemoHtml : public DemoBase, public view::ButtonListener
{
public:
    explicit DemoHtml(DemoMain* main);
    virtual ~DemoHtml();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from view::ButtonListener:
    virtual void ButtonPressed(view::Button* sender,
        const view::Event& event);

private:
    view::HtmlView* html_view_;
    view::Textfield* html_;
    view::NativeTextButton* button_;

    DISALLOW_COPY_AND_ASSIGN(DemoHtml);
};

#endif //__demo_html_h__