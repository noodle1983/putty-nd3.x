
#ifndef __view_native_widget_views_h__
#define __view_native_widget_views_h__

#pragma once

#include "base/message_loop.h"

#include "ui_gfx/transform.h"

#include "native_widget_private.h"

namespace view
{
    namespace internal
    {
        class NativeWidgetView;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // NativeWidgetViews
    //
    //  A NativeWidget implementation that uses another View as its native widget.
    class NativeWidgetViews : public internal::NativeWidgetPrivate
    {
    public:
        explicit NativeWidgetViews(internal::NativeWidgetDelegate* delegate);
        virtual ~NativeWidgetViews();

        // TODO(beng): remove.
        View* GetView();
        const View* GetView() const;

        void OnActivate(bool active);

        internal::NativeWidgetDelegate* delegate() { return delegate_; }

    protected:
        // Overridden from internal::NativeWidgetPrivate:
        virtual void InitNativeWidget(const Widget::InitParams& params);
        virtual NonClientFrameView* CreateNonClientFrameView();
        virtual void UpdateFrameAfterFrameChange();
        virtual bool ShouldUseNativeFrame() const;
        virtual void FrameTypeChanged();
        virtual Widget* GetWidget();
        virtual const Widget* GetWidget() const;
        virtual HWND GetNativeView() const;
        virtual HWND GetNativeWindow() const;
        virtual const ui::Compositor* GetCompositor() const;
        virtual ui::Compositor* GetCompositor();
        virtual void MarkLayerDirty();
        virtual void CalculateOffsetToAncestorWithLayer(gfx::Point* offset,
            View** ancestor);
        virtual void ViewRemoved(View* view);
        virtual void SetNativeWindowProperty(const char* name, void* value);
        virtual void* GetNativeWindowProperty(const char* name) const;
        virtual TooltipManager* GetTooltipManager() const;
        virtual bool IsScreenReaderActive() const;
        virtual void SendNativeAccessibilityEvent(View* view,
            ui::AccessibilityTypes::Event event_type);
        virtual void SetMouseCapture();
        virtual void ReleaseMouseCapture();
        virtual bool HasMouseCapture() const;
        virtual void CenterWindow(const gfx::Size& size);
        virtual void GetWindowBoundsAndMaximizedState(gfx::Rect* bounds,
            bool* maximized) const;
        virtual void SetWindowTitle(const std::wstring& title);
        virtual void SetWindowIcons(const SkBitmap& window_icon,
            const SkBitmap& app_icon);
        virtual void SetAccessibleName(const std::wstring& name);
        virtual void SetAccessibleRole(ui::AccessibilityTypes::Role role);
        virtual void SetAccessibleState(ui::AccessibilityTypes::State state);
        virtual void BecomeModal();
        virtual gfx::Rect GetWindowScreenBounds() const;
        virtual gfx::Rect GetClientAreaScreenBounds() const;
        virtual gfx::Rect GetRestoredBounds() const;
        virtual void SetBounds(const gfx::Rect& bounds);
        virtual void SetSize(const gfx::Size& size);
        virtual void SetBoundsConstrained(const gfx::Rect& bounds,
            Widget* other_widget);
        virtual void MoveAbove(HWND native_view);
        virtual void MoveToTop();
        virtual void SetShape(HRGN shape);
        virtual void Close();
        virtual void CloseNow();
        virtual void EnableClose(bool enable);
        virtual void Show();
        virtual void Hide();
        virtual void ShowNativeWidget(ShowState state);
        virtual bool IsVisible() const;
        virtual void Activate();
        virtual void Deactivate();
        virtual bool IsActive() const;
        virtual void SetAlwaysOnTop(bool always_on_top);
        virtual void Maximize();
        virtual void Minimize();
        virtual bool IsMaximized() const;
        virtual bool IsMinimized() const;
        virtual void Restore();
        virtual void SetFullscreen(bool fullscreen);
        virtual bool IsFullscreen() const;
        virtual void SetOpacity(unsigned char opacity);
        virtual void SetUseDragFrame(bool use_drag_frame);
        virtual bool IsAccessibleWidget() const;
        virtual bool ContainsNativeView(HWND native_view) const;
        virtual void RunShellDrag(View* view,
            const ui::OSExchangeData& data,
            int operation);
        virtual void SchedulePaintInRect(const gfx::Rect& rect);
        virtual void SetCursor(HCURSOR cursor);

    private:
        internal::NativeWidgetPrivate* GetParentNativeWidget();
        const internal::NativeWidgetPrivate* GetParentNativeWidget() const;

        internal::NativeWidgetDelegate* delegate_;

        internal::NativeWidgetView* view_;

        bool active_;

        bool minimized_;

        // The following factory is used for calls to close the NativeWidgetViews
        // instance.
        ScopedRunnableMethodFactory<NativeWidgetViews> close_widget_factory_;

        gfx::Rect restored_bounds_;
        gfx::Transform restored_transform_;

        Widget* hosting_widget_;

        DISALLOW_COPY_AND_ASSIGN(NativeWidgetViews);
    };

} //namespace view

#endif //__view_native_widget_views_h__