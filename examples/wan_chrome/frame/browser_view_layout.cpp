
#include "browser_view_layout.h"

#include "gfx/size.h"

#include "browser_view.h"

////////////////////////////////////////////////////////////////////////////////
// BrowserViewLayout, public:

BrowserViewLayout::BrowserViewLayout()
: contents_container_(NULL),
browser_view_(NULL)
{
}

BrowserViewLayout::~BrowserViewLayout() {}

gfx::Size BrowserViewLayout::GetMinimumSize()
{
    return gfx::Size(400, 400);
}

int BrowserViewLayout::NonClientHitTest(const gfx::Point& point)
{
    // If the point is somewhere else, delegate to the default implementation.
    return browser_view_->NonClientHitTest(point);
}

//////////////////////////////////////////////////////////////////////////////
// BrowserViewLayout, view::LayoutManager implementation:

void BrowserViewLayout::Installed(view::View* host)
{
    contents_container_ = NULL;
    browser_view_ = static_cast<BrowserView*>(host);
}

void BrowserViewLayout::Uninstalled(view::View* host) {}

void BrowserViewLayout::ViewAdded(view::View* host, view::View* view)
{
}

void BrowserViewLayout::ViewRemoved(view::View* host, view::View* view)
{
}

void BrowserViewLayout::Layout(view::View* host)
{
}

// Return the preferred size which is the size required to give each
// children their respective preferred size.
gfx::Size BrowserViewLayout::GetPreferredSize(view::View* host)
{
    return gfx::Size();
}