
#include "menu_host_win.h"

#include "menu_host_view.h"
#include "native_menu_host_delegate.h"
#include "view/view_delegate.h"

namespace view
{

    ////////////////////////////////////////////////////////////////////////////////
    // MenuHostWin, public:

    MenuHostWin::MenuHostWin(internal::NativeMenuHostDelegate* delegate)
        : NativeWidgetWin(delegate->AsNativeWidgetDelegate()),
        delegate_(delegate) {}

    MenuHostWin::~MenuHostWin() {}

    ////////////////////////////////////////////////////////////////////////////////
    // MenuHostWin, NativeMenuHost implementation:

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
        NativeWidgetWin::OnDestroy();
    }

    void MenuHostWin::OnCancelMode()
    {
        delegate_->OnNativeMenuHostCancelCapture();
        NativeWidgetWin::OnCancelMode();
    }
    ////////////////////////////////////////////////////////////////////////////////
    // NativeMenuHost, public:

    // static
    NativeMenuHost* NativeMenuHost::CreateNativeMenuHost(
        internal::NativeMenuHostDelegate* delegate)
    {
        if(Widget::IsPureViews() &&
            ViewDelegate::view_delegate->GetDefaultParentView())
        {
            return new MenuHostView(delegate);
        }

        return new MenuHostWin(delegate);
    }

} //namespace view