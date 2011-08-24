
#ifndef __demo_activex_h__
#define __demo_activex_h__

#pragma once

#include "view/controls/button/button.h"

#include "demo_base.h"

namespace view
{
    class FlashView;
    class MediaPlayerView;
    class NativeTextButton;
}

class DemoActiveX : public DemoBase, public view::ButtonListener
{
public:
    explicit DemoActiveX(DemoMain* main);
    virtual ~DemoActiveX();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from view::ButtonListener:
    virtual void ButtonPressed(view::Button* sender,
        const view::Event& event);

private:
    view::FlashView* flash_view_;
    view::MediaPlayerView* media_player_;
    view::NativeTextButton* play_flash_;
    view::NativeTextButton* play_wmv_;
    view::NativeTextButton* create_flash_popup_;

    DISALLOW_COPY_AND_ASSIGN(DemoActiveX);
};

#endif //__demo_activex_h__