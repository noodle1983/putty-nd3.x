
#include "browser_frame_win.h"

#include <dwmapi.h>

#include "view/view/root_view.h"

#include "browser_view.h"
#include "opaque_browser_frame_view.h"

// static
static const int kClientEdgeThickness = 3;
// If not -1, windows are shown with this state.
static int explicit_show_state = -1;

///////////////////////////////////////////////////////////////////////////////
// BrowserFrameWin, public:

BrowserFrameWin::BrowserFrameWin(BrowserView* browser_view)
: WindowWin(browser_view),
BrowserFrame(browser_view),
browser_view_(browser_view),
delegate_(this)
{
    set_native_browser_frame(this);
    browser_view_->set_frame(this);
    non_client_view()->SetFrameView(CreateFrameViewForWindow());
    // Don't focus anything on creation, selecting a tab will set the focus.
    set_focus_on_creation(false);
}

BrowserFrameWin::~BrowserFrameWin() {}

void BrowserFrameWin::InitBrowserFrame()
{
    WindowWin::Init(NULL, gfx::Rect());
}

// static
void BrowserFrameWin::SetShowState(int state)
{
    explicit_show_state = state;
}

///////////////////////////////////////////////////////////////////////////////
// BrowserFrameWin, view::WindowWin overrides:

int BrowserFrameWin::GetShowState() const
{
    if(explicit_show_state != -1)
    {
        return explicit_show_state;
    }

    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    GetStartupInfo(&si);
    return si.wShowWindow;
}

gfx::Insets BrowserFrameWin::GetClientAreaInsets() const
{
    if(!non_client_view()->UseNativeFrame())
    {
        return WindowWin::GetClientAreaInsets();
    }

    int border_thickness = GetSystemMetrics(SM_CXSIZEFRAME);
    // In fullscreen mode, we have no frame. In restored mode, we draw our own
    // client edge over part of the default frame.
    if(IsFullscreen())
    {
        border_thickness = 0;
    }
    else if(!IsMaximized())
    {
        border_thickness -= kClientEdgeThickness;
    }
    return gfx::Insets(0, border_thickness, border_thickness, border_thickness);
}

///////////////////////////////////////////////////////////////////////////////
// BrowserFrameWin, view::Window overrides:

void BrowserFrameWin::Activate()
{
    WindowWin::Activate();
}

void BrowserFrameWin::UpdateFrameAfterFrameChange()
{
    // We need to update the glass region on or off before the base class adjusts
    // the window region.
    UpdateDWMFrame();
    WindowWin::UpdateFrameAfterFrameChange();
}

view::RootView* BrowserFrameWin::CreateRootView()
{
    return delegate_->DelegateCreateRootView();
}

view::NonClientFrameView* BrowserFrameWin::CreateFrameViewForWindow()
{
    return delegate_->DelegateCreateFrameViewForWindow();
}

////////////////////////////////////////////////////////////////////////////////
// BrowserFrameWin, NativeBrowserFrame implementation:

view::NativeWindow* BrowserFrameWin::AsNativeWindow()
{
    return this;
}

const view::NativeWindow* BrowserFrameWin::AsNativeWindow() const
{
    return this;
}

BrowserNonClientFrameView* BrowserFrameWin::CreateBrowserNonClientFrameView()
{
    //if(AlwaysUseNativeFrame())
    //{
    //    return new GlassBrowserFrameView(this, browser_view_);
    //}
    return new OpaqueBrowserFrameView(this, browser_view_);
}

int BrowserFrameWin::GetMinimizeButtonOffset() const
{
    TITLEBARINFOEX titlebar_info;
    titlebar_info.cbSize = sizeof(TITLEBARINFOEX);
    SendMessage(GetNativeView(), WM_GETTITLEBARINFOEX, 0, (WPARAM)&titlebar_info);

    POINT minimize_button_corner =
    {
        titlebar_info.rgrect[2].left,
        titlebar_info.rgrect[2].top
    };
    MapWindowPoints(HWND_DESKTOP, GetNativeView(), &minimize_button_corner, 1);

    return minimize_button_corner.x;
}

bool BrowserFrameWin::AlwaysUseNativeFrame() const
{
    return view::WidgetWin::IsAeroGlassEnabled();
}

void BrowserFrameWin::TabStripDisplayModeChanged()
{
    if(GetRootView()->has_children())
    {
        // Make sure the child of the root view gets Layout again.
        GetRootView()->GetChildViewAt(0)->InvalidateLayout();
    }
    GetRootView()->Layout();

    UpdateDWMFrame();
}

///////////////////////////////////////////////////////////////////////////////
// BrowserFrameWin, private:

void BrowserFrameWin::UpdateDWMFrame()
{
    // Nothing to do yet, or we're not showing a DWM frame.
    if(!client_view() || !AlwaysUseNativeFrame())
    {
        return;
    }

    MARGINS margins = { 0 };
    {
        // In fullscreen mode, we don't extend glass into the client area at all,
        // because the GDI-drawn text in the web content composited over it will
        // become semi-transparent over any glass area.
        if(!IsMaximized() && !IsFullscreen())
        {
            margins.cxLeftWidth = kClientEdgeThickness + 1;
            margins.cxRightWidth = kClientEdgeThickness + 1;
            margins.cyBottomHeight = kClientEdgeThickness + 1;
            margins.cyTopHeight = kClientEdgeThickness + 1;
        }
    }
    DwmExtendFrameIntoClientArea(GetNativeView(), &margins);
}