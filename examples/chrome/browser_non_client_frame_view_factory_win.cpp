
#include "browser_non_client_frame_view.h"

#include "browser_view.h"
#include "opaque_browser_frame_view.h"

namespace browser
{

    BrowserNonClientFrameView* CreateBrowserNonClientFrameView(
        BrowserFrame* frame, BrowserView* browser_view)
    {
        if(browser_view->IsBrowserTypePanel())
        {
            //return new PanelBrowserFrameView(
            //    frame, static_cast<PanelBrowserView*>(browser_view));
            return NULL;
        }
        else
        {
            return new OpaqueBrowserFrameView(frame, browser_view);
        }
    }

} //namespace browser