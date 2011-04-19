
#include "browser_frame.h"

#include "gfx/font.h"

#include "browser_frame_win.h"
#include "browser_non_client_frame_view.h"
#include "browser_root_view.h"

////////////////////////////////////////////////////////////////////////////////
// BrowserFrame, public:

BrowserFrame::~BrowserFrame() {}

// static (Factory method.)
BrowserFrame* BrowserFrame::Create(BrowserView* browser_view)
{
    BrowserFrameWin* frame = new BrowserFrameWin(browser_view);
    frame->InitBrowserFrame();
    return frame;
}

// static
const gfx::Font& BrowserFrame::GetTitleFont()
{
    static gfx::Font* title_font =
        new gfx::Font(view::WindowWin::GetWindowTitleFont());
    return *title_font;
}

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
    browser_frame_view_ =
        native_browser_frame_->CreateBrowserNonClientFrameView();
    return browser_frame_view_;
}

////////////////////////////////////////////////////////////////////////////////
// BrowserFrame, protected:

BrowserFrame::BrowserFrame(BrowserView* browser_view)
: native_browser_frame_(NULL),
root_view_(NULL),
browser_frame_view_(NULL),
browser_view_(browser_view) {}