
#include "browser_root_view.h"

BrowserRootView::BrowserRootView(BrowserView* browser_view, view::Widget* widget)
: view::RootView(widget), browser_view_(browser_view) {}