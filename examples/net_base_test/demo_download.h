
#ifndef __demo_download_h__
#define __demo_download_h__

#pragma once

#include "view/controls/button/button.h"
#include "net_base/net_base_sdk.h"

#include "demo_base.h"

namespace view
{
    class TextButton;
}

class DemoDownload : public DemoBase, public view::ButtonListener
{
public:
    explicit DemoDownload(DemoMain* main);
    virtual ~DemoDownload();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from view::ButtonListener:
    virtual void ButtonPressed(view::Button* sender,
        const view::Event& event);

private:
    IDownloadEngine* download_engine_;

    view::TextButton* start_stop_;

    DISALLOW_COPY_AND_ASSIGN(DemoDownload);
};

#endif //__demo_download_h__