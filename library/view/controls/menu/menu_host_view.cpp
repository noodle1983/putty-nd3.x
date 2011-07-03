
#include "menu_host_view.h"

#include "native_menu_host_delegate.h"

namespace view
{

    MenuHostView::MenuHostView(internal::NativeMenuHostDelegate* delegate)
        : NativeWidgetViews(delegate->AsNativeWidgetDelegate()),
        delegate_(delegate) {}

    MenuHostView::~MenuHostView() {}

    void MenuHostView::StartCapturing()
    {
        SetMouseCapture();
    }

    NativeWidget* MenuHostView::AsNativeWidget()
    {
        return this;
    }

    void MenuHostView::InitNativeWidget(const Widget::InitParams& params)
    {
        NativeWidgetViews::InitNativeWidget(params);
        GetView()->SetVisible(false);
    }

} //namespace view