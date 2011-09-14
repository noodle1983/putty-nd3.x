
#include "browser_frame_views.h"

////////////////////////////////////////////////////////////////////////////////
// BrowserFrameViews, public:

BrowserFrameViews::BrowserFrameViews(BrowserFrame* browser_frame,
                                     BrowserView* browser_view)
                                     : view::NativeWidgetViews(browser_frame),
                                     browser_view_(browser_view),
                                     browser_frame_(browser_frame) {}

BrowserFrameViews::~BrowserFrameViews() {}

////////////////////////////////////////////////////////////////////////////////
// BrowserFrameViews, NativeBrowserFrame implementation:

view::NativeWidget* BrowserFrameViews::AsNativeWidget()
{
    return this;
}

const view::NativeWidget* BrowserFrameViews::AsNativeWidget() const
{
    return this;
}

int BrowserFrameViews::GetMinimizeButtonOffset() const
{
    return 0;
}

void BrowserFrameViews::TabStripDisplayModeChanged() {}