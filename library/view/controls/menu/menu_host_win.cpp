
#include "menu_host_win.h"

#include "native_menu_host_delegate.h"

namespace view
{

    ////////////////////////////////////////////////////////////////////////////////
    // MenuHostWin, public:

    MenuHostWin::MenuHostWin(NativeMenuHostDelegate* delegate)
        : delegate_(delegate) {}

    MenuHostWin::~MenuHostWin() {}

    ////////////////////////////////////////////////////////////////////////////////
    // MenuHostWin, NativeMenuHost implementation:

    void MenuHostWin::InitMenuHost(HWND parent,
        const gfx::Rect& bounds)
    {
        WidgetWin::Init(parent, bounds);
    }

    void MenuHostWin::StartCapturing()
    {
        SetMouseCapture();
    }

    NativeWidget* MenuHostWin::AsNativeWidget()
    {
        return this;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // MenuHostWin, WidgetWin overrides:

    void MenuHostWin::OnDestroy()
    {
        delegate_->OnNativeMenuHostDestroy();
        WidgetWin::OnDestroy();
    }

    void MenuHostWin::OnCancelMode()
    {
        delegate_->OnNativeMenuHostCancelCapture();
        WidgetWin::OnCancelMode();
    }

    // TODO(beng): remove once MenuHost is-a Widget
    RootView* MenuHostWin::CreateRootView()
    {
        return delegate_->CreateRootView();
    }

    // TODO(beng): remove once MenuHost is-a Widget
    bool MenuHostWin::ShouldReleaseCaptureOnMouseReleased() const
    {
        return delegate_->ShouldReleaseCaptureOnMouseRelease();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeMenuHost, public:

    // static
    NativeMenuHost* NativeMenuHost::CreateNativeMenuHost(
        NativeMenuHostDelegate* delegate)
    {
        return new MenuHostWin(delegate);
    }

} //namespace view