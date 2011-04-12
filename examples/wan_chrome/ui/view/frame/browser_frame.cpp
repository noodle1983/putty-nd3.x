
#include "browser_frame.h"

#include "view/widget/native_widget.h"
#include "view/window/native_window.h"

#include "browser_non_client_frame_view.h"
#include "browser_root_view.h"
#include "native_browser_frame.h"

////////////////////////////////////////////////////////////////////////////////
// BrowserFrame, public:

BrowserFrame::~BrowserFrame() {}

view::Window* BrowserFrame::GetWindow()
{
    return native_browser_frame_->AsNativeWindow()->GetWindow();
}

int BrowserFrame::GetMinimizeButtonOffset() const
{
    return native_browser_frame_->GetMinimizeButtonOffset();
}

gfx::Rect BrowserFrame::GetBoundsForTabStrip(view::View* tabstrip) const
{
    return browser_frame_view_->GetBoundsForTabStrip(tabstrip);
}

int BrowserFrame::GetHorizontalTabStripVerticalOffset(bool restored) const
{
    return browser_frame_view_->GetHorizontalTabStripVerticalOffset(restored);
}

void BrowserFrame::UpdateThrobber(bool running)
{
    browser_frame_view_->UpdateThrobber(running);
}

ThemeProvider* BrowserFrame::GetThemeProviderForFrame() const
{
    return native_browser_frame_->GetThemeProviderForFrame();
}

bool BrowserFrame::AlwaysUseNativeFrame() const
{
    return native_browser_frame_->AlwaysUseNativeFrame();
}

view::View* BrowserFrame::GetFrameView() const
{
    return browser_frame_view_;
}

void BrowserFrame::TabStripDisplayModeChanged()
{
    native_browser_frame_->TabStripDisplayModeChanged();
}

////////////////////////////////////////////////////////////////////////////////
// BrowserFrame, NativeBrowserFrameDelegate implementation:

view::RootView* BrowserFrame::DelegateCreateRootView()
{
    root_view_ = new BrowserRootView(browser_view_,
        native_browser_frame_->AsNativeWindow()->AsNativeWidget()->GetWidget());
    return root_view_;
}

view::NonClientFrameView* BrowserFrame::DelegateCreateFrameViewForWindow()
{
    browser_frame_view_ = native_browser_frame_->CreateBrowserNonClientFrameView();
    return browser_frame_view_;
}


////////////////////////////////////////////////////////////////////////////////
// BrowserFrame, protected:

BrowserFrame::BrowserFrame(BrowserView* browser_view)
: native_browser_frame_(NULL),
root_view_(NULL),
browser_frame_view_(NULL),
browser_view_(browser_view) {}