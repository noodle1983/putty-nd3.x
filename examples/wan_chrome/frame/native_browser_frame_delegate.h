
#ifndef __wan_chrome_frame_native_browser_frame_delegate_h__
#define __wan_chrome_frame_native_browser_frame_delegate_h__

#pragma once

namespace view
{
    class NonClientFrameView;
    class RootView;
}

class NativeBrowserFrameDelegate
{
public:
    virtual ~NativeBrowserFrameDelegate() {}

    virtual view::RootView* DelegateCreateRootView() = 0;
    virtual view::NonClientFrameView* DelegateCreateFrameViewForWindow() = 0;
};

#endif //__wan_chrome_frame_native_browser_frame_delegate_h__