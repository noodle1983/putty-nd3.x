
#ifndef __demo_webbrowser_h__
#define __demo_webbrowser_h__

#pragma once

#include "view/controls/button/button.h"

#include "demo_base.h"

namespace view
{
    class NativeTextButton;
    class Textfield;
    class WebBrowserView;
}

class DemoWebBrowser : public DemoBase, public view::ButtonListener
{
public:
    explicit DemoWebBrowser(DemoMain* main);
    virtual ~DemoWebBrowser();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from view::ButtonListener:
    virtual void ButtonPressed(view::Button* sender,
        const view::Event& event);

private:
    view::WebBrowserView* web_view_;
    view::Textfield* web_address_;
    view::NativeTextButton* button_;

    DISALLOW_COPY_AND_ASSIGN(DemoWebBrowser);
};

#endif //__demo_webbrowser_h__