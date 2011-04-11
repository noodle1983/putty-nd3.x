
#include "app_panel_browser_frame_view.h"
#include "browser_view.h"
#include "opaque_browser_frame_view.h"

BrowserNonClientFrameView* CreateBrowserNonClientFrameView(
    BrowserFrame* frame, BrowserView* browser_view)
{
    if(browser_view->IsBrowserTypePanel())
    {
        return new AppPanelBrowserFrameView(frame, browser_view);
    }
    else
    {
        return new OpaqueBrowserFrameView(frame, browser_view);
    }
}