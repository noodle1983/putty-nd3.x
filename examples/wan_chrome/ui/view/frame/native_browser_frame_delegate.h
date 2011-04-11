
#ifndef __wan_chrome_ui_view_frame_native_browser_frame_delegate_h__
#define __wan_chrome_ui_view_frame_native_browser_frame_delegate_h__

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

    // TODO(beng): Remove these once BrowserFrame is-a Window is-a Widget, at
    //             which point BrowserFrame can just override Widget's method.
    virtual view::RootView* DelegateCreateRootView() = 0;
    virtual view::NonClientFrameView* DelegateCreateFrameViewForWindow() = 0;
};

#endif //__wan_chrome_ui_view_frame_native_browser_frame_delegate_h__