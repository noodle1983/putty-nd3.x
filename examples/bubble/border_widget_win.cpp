
#include "border_widget_win.h"

#include "view/widget/widget.h"

#include "border_content.h"

BorderWidgetWin::BorderWidgetWin()
: view::NativeWidgetWin(new view::Widget),
border_content_(NULL) {}

void BorderWidgetWin::InitBorderWidgetWin(BorderContent* border_content,
                                          HWND owner)
{
    DCHECK(!border_content_);
    border_content_ = border_content;
    border_content_->Init();

    view::Widget::InitParams params(view::Widget::InitParams::TYPE_POPUP);
    params.transparent = true;
    params.parent = owner;
    params.native_widget = this;
    GetWidget()->Init(params);
    GetWidget()->SetContentsView(border_content_);
    SetWindowPos(owner, 0, 0, 0, 0,
        SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOREDRAW);
}

gfx::Rect BorderWidgetWin::SizeAndGetBounds(
    const gfx::Rect& position_relative_to,
    BubbleBorder::ArrowLocation arrow_location,
    const gfx::Size& content_size)
{
    // Ask the border view to calculate our bounds (and our content').
    gfx::Rect content_bounds;
    gfx::Rect window_bounds;
    border_content_->SizeAndGetBounds(position_relative_to, arrow_location,
        false, content_size, &content_bounds, &window_bounds);
    GetWidget()->SetBounds(window_bounds);

    // Return |content_bounds| in screen coordinates.
    content_bounds.Offset(window_bounds.origin());
    return content_bounds;
}

LRESULT BorderWidgetWin::OnMouseActivate(UINT message,
                                         WPARAM w_param,
                                         LPARAM l_param)
{
    // Never activate.
    return MA_NOACTIVATE;
}