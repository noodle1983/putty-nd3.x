
#include "browser_frame.h"

#include "view/widget/native_widget.h"

#include "browser_list.h"
#include "browser_root_view.h"
#include "browser_view.h"
#include "glass_browser_frame_view.h"

////////////////////////////////////////////////////////////////////////////////
// BrowserFrame, public:

BrowserFrame::BrowserFrame(BrowserView* browser_view)
: native_browser_frame_(NULL),
root_view_(NULL),
browser_frame_view_(NULL),
browser_view_(browser_view)
{
    browser_view_->set_frame(this);
    set_is_secondary_widget(false);
    // Don't focus anything on creation, selecting a tab will set the focus.
    set_focus_on_creation(false);
}

BrowserFrame::~BrowserFrame() {}

void BrowserFrame::InitBrowserFrame()
{
    native_browser_frame_ =
        NativeBrowserFrame::CreateNativeBrowserFrame(this, browser_view_);
    view::Widget::InitParams params;
    params.delegate = browser_view_;
    params.native_widget = native_browser_frame_->AsNativeWidget();
    if(browser_view_->browser()->is_type_tabbed())
    {
        // Typed panel/popup can only return a size once the widget has been
        // created.
        params.bounds = browser_view_->browser()->GetSavedWindowBounds();
        params.show_state = browser_view_->browser()->GetSavedWindowShowState();
    }
    Init(params);
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

view::View* BrowserFrame::GetFrameView() const
{
    return browser_frame_view_;
}

void BrowserFrame::TabStripDisplayModeChanged()
{
    if(GetRootView()->has_children())
    {
        // Make sure the child of the root view gets Layout again.
        GetRootView()->child_at(0)->InvalidateLayout();
    }
    GetRootView()->Layout();
    native_browser_frame_->TabStripDisplayModeChanged();
}

///////////////////////////////////////////////////////////////////////////////
// BrowserFrameWin, view::Window overrides:

bool BrowserFrame::IsMaximized() const
{
    return Widget::IsMaximized();
}

view::internal::RootView* BrowserFrame::CreateRootView()
{
    root_view_ = new BrowserRootView(browser_view_, this);
    return root_view_;
}

view::NonClientFrameView* BrowserFrame::CreateNonClientFrameView()
{
    if(ShouldUseNativeFrame())
    {
        browser_frame_view_ = new GlassBrowserFrameView(this, browser_view_);
    }
    else
    {
        browser_frame_view_ =
            browser::CreateBrowserNonClientFrameView(this, browser_view_);
    }
    return browser_frame_view_;
}

bool BrowserFrame::GetAccelerator(int command_id,
                                  ui::Accelerator* accelerator)
{
    return browser_view_->GetAccelerator(command_id, accelerator);
}

//ui::ThemeProvider* BrowserFrame::GetThemeProvider() const
//{
//    return ThemeServiceFactory::GetForProfile(
//        browser_view_->browser()->profile());
//}

void BrowserFrame::OnNativeWidgetActivationChanged(bool active)
{
    if(active)
    {
        // When running under remote desktop, if the remote desktop client is not
        // active on the users desktop, then none of the windows contained in the
        // remote desktop will be activated.  However, NativeWidgetWin::Activate()
        // will still bring this browser window to the foreground.  We explicitly
        // set ourselves as the last active browser window to ensure that we get
        // treated as such by the rest of Chrome.
        BrowserList::SetLastActive(browser_view_->browser());
    }
    Widget::OnNativeWidgetActivationChanged(active);
}