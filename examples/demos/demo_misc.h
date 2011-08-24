
#ifndef __demo_misc_h__
#define __demo_misc_h__

#pragma once

#include "view/controls/button/button.h"

#include "demo_base.h"

namespace view
{
    class CheckmarkThrobber;
    class NativeTextButton;
    class ProgressBar;
    class SingleSplitView;
}

class DemoMisc : public DemoBase, public view::ButtonListener
{
public:
    explicit DemoMisc(DemoMain* main);
    virtual ~DemoMisc();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from view::ButtonListener:
    virtual void ButtonPressed(view::Button* sender,
        const view::Event& event);

private:
    view::CheckmarkThrobber* checkmark_throbber_;
    view::NativeTextButton* checkmark_complete_;
    view::ProgressBar* progress_;
    view::NativeTextButton* progress_del_;
    view::NativeTextButton* progress_add_;
    view::SingleSplitView* split_view_;

    DISALLOW_COPY_AND_ASSIGN(DemoMisc);
};

#endif //__demo_misc_h__