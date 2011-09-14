
#ifndef __native_tab_contents_view_delegate_h__
#define __native_tab_contents_view_delegate_h__

#pragma once

namespace gfx
{
    class Size;
}

namespace view
{
    namespace internal
    {
        class NativeWidgetDelegate;
    }
}
class TabContents;

namespace internal
{

    class NativeTabContentsViewDelegate
    {
    public:
        virtual ~NativeTabContentsViewDelegate() {}

        virtual TabContents* GetTabContents() = 0;

        // TODO(beng):
        // This can die with OnNativeTabContentsViewMouseDown/Move().
        virtual bool IsShowingSadTab() const = 0;

        // TODO(beng):
        // These three can be replaced by Widget::OnSizeChanged and some new
        // notifications for show/hide.
        virtual void OnNativeTabContentsViewShown() = 0;
        virtual void OnNativeTabContentsViewHidden() = 0;
        virtual void OnNativeTabContentsViewSized(const gfx::Size& size) = 0;

        virtual void OnNativeTabContentsViewWheelZoom(bool zoom_in) = 0;

        // TODO(beng):
        // These two can be replaced by an override of Widget::OnMouseEvent.
        virtual void OnNativeTabContentsViewMouseDown() = 0;
        virtual void OnNativeTabContentsViewMouseMove(bool motion) = 0;

        virtual void OnNativeTabContentsViewDraggingEnded() = 0;

        virtual view::internal::NativeWidgetDelegate* AsNativeWidgetDelegate() = 0;
    };

} //namespace internal

#endif //__native_tab_contents_view_delegate_h__