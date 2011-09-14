
#ifndef __native_tab_contents_view_h__
#define __native_tab_contents_view_h__

#pragma once

#include <windows.h>

#include <string>

class RenderWidgetHost;
class RenderWidgetHostView;
//struct WebDropData;
namespace gfx
{
    class Point;
}

namespace view
{
    class NativeWidget;
}

namespace internal
{
    class NativeTabContentsViewDelegate;
}

class NativeTabContentsView
{
public:
    virtual ~NativeTabContentsView() {}

    static NativeTabContentsView* CreateNativeTabContentsView(
        internal::NativeTabContentsViewDelegate* delegate);

    virtual void InitNativeTabContentsView() = 0;

    virtual void Unparent() = 0;

    virtual RenderWidgetHostView* CreateRenderWidgetHostView(
        RenderWidgetHost* render_widget_host) = 0;

    virtual HWND GetTopLevelNativeWindow() const = 0;

    virtual void SetPageTitle(const std::wstring& title) = 0;

    //virtual void StartDragging(const WebDropData& drop_data,
    //    WebKit::WebDragOperationsMask ops,
    //    const SkBitmap& image,
    //    const gfx::Point& image_offset) = 0;
    //virtual void CancelDrag() = 0;
    //virtual bool IsDoingDrag() const = 0;
    //virtual void SetDragCursor(WebKit::WebDragOperation operation) = 0;

    virtual view::NativeWidget* AsNativeWidget() = 0;
};

#endif //__native_tab_contents_view_h__