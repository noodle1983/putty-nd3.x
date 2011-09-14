
#ifndef __tab_contents_view_views_h__
#define __tab_contents_view_views_h__

#pragma once

#include "base/memory/scoped_ptr.h"
#include "base/timer.h"

#include "view/widget/widget.h"

#include "native_tab_contents_view_delegate.h"
#include "tab_contents_view.h"

class SkBitmap;
namespace gfx
{
    class Point;
    class Size;
}

class NativeTabContentsView;
class SadTabView;

// Views-specific implementation of the TabContentsView.
// TODO(beng): Remove last remnants of Windows-specificity, and make this
//             subclass Widget.
class TabContentsViewViews : public view::Widget,
    public TabContentsView,
    public internal::NativeTabContentsViewDelegate
{
public:
    // The corresponding TabContents is passed in the constructor, and manages our
    // lifetime. This doesn't need to be the case, but is this way currently
    // because that's what was easiest when they were split.
    explicit TabContentsViewViews(TabContents* tab_contents);
    virtual ~TabContentsViewViews();

    // Reset the native parent of this view to NULL.  Unparented windows should
    // not receive any messages.
    virtual void Unparent();

    NativeTabContentsView* native_tab_contents_view() const
    {
        return native_tab_contents_view_;
    }

    // Overridden from TabContentsView:
    virtual void CreateView(const gfx::Size& initial_size);
    virtual RenderWidgetHostView* CreateViewForWidget(
        RenderWidgetHost* render_widget_host);
    virtual HWND GetNativeView() const;
    virtual HWND GetContentNativeView() const;
    virtual HWND GetTopLevelNativeWindow() const;
    virtual void GetContainerBounds(gfx::Rect* out) const;
    virtual void SetPageTitle(const std::wstring& title);
    virtual void OnTabCrashed(base::TerminationStatus status,
        int error_code);
    virtual void SizeContents(const gfx::Size& size);
    virtual void RenderViewCreated(RenderViewHost* host);
    virtual void Focus();
    virtual void SetInitialFocus();
    virtual void StoreFocus();
    virtual void RestoreFocus();
    virtual void UpdatePreferredSize(const gfx::Size& pref_size);
    virtual bool IsDoingDrag() const;
    virtual void CancelDragAndCloseTab();
    virtual bool IsEventTracking() const;
    virtual void CloseTabAfterEventTracking();
    virtual void GetViewBounds(gfx::Rect* out) const;

    // Implementation of RenderViewHostDelegate::View.
    //virtual void CreateNewWindow(
    //    int route_id,
    //    const ViewHostMsg_CreateWindow_Params& params);
    //virtual void CreateNewWidget(int route_id,
    //    WebKit::WebPopupType popup_type);
    //virtual void CreateNewFullscreenWidget(int route_id);
    //virtual void ShowCreatedWindow(int route_id,
    //    WindowOpenDisposition disposition,
    //    const gfx::Rect& initial_pos,
    //    bool user_gesture);
    //virtual void ShowCreatedWidget(int route_id,
    //    const gfx::Rect& initial_pos);
    //virtual void ShowCreatedFullscreenWidget(int route_id);
    //virtual void ShowContextMenu(const ContextMenuParams& params);
    //virtual void ShowPopupMenu(const gfx::Rect& bounds,
    //    int item_height,
    //    double item_font_size,
    //    int selected_item,
    //    const std::vector<WebMenuItem>& items,
    //    bool right_aligned);
    //virtual void StartDragging(const WebDropData& drop_data,
    //    WebKit::WebDragOperationsMask operations,
    //    const SkBitmap& image,
    //    const gfx::Point& image_offset);
    //virtual void UpdateDragCursor(WebKit::WebDragOperation operation);
    //virtual void GotFocus();
    //virtual void TakeFocus(bool reverse);

private:
    // Overridden from internal::NativeTabContentsViewDelegate:
    virtual TabContents* GetTabContents() OVERRIDE;
    virtual bool IsShowingSadTab() const OVERRIDE;
    virtual void OnNativeTabContentsViewShown() OVERRIDE;
    virtual void OnNativeTabContentsViewHidden() OVERRIDE;
    virtual void OnNativeTabContentsViewSized(const gfx::Size& size) OVERRIDE;
    virtual void OnNativeTabContentsViewWheelZoom(bool zoom_in) OVERRIDE;
    virtual void OnNativeTabContentsViewMouseDown() OVERRIDE;
    virtual void OnNativeTabContentsViewMouseMove(bool motion) OVERRIDE;
    virtual void OnNativeTabContentsViewDraggingEnded() OVERRIDE;
    virtual view::internal::NativeWidgetDelegate* AsNativeWidgetDelegate()
        OVERRIDE;

    // Overridden from view::Widget:
    virtual view::FocusManager* GetFocusManager() OVERRIDE;

    // A helper method for closing the tab.
    void CloseTab();

    // Windows events ------------------------------------------------------------

    // Handles notifying the TabContents and other operations when the window was
    // shown or hidden.
    void WasHidden();
    void WasShown();

    // Handles resizing of the contents. This will notify the RenderWidgetHostView
    // of the change, reposition popups, and the find in page bar.
    void WasSized(const gfx::Size& size);

    // TODO(brettw) comment these. They're confusing.
    void WheelZoom(int distance);

    // ---------------------------------------------------------------------------

    // The TabContents whose contents we display.
    TabContents* tab_contents_;

    // Common implementations of some RenderViewHostDelegate::View methods.
    //RenderViewHostDelegateViewHelper delegate_view_helper_;

    NativeTabContentsView* native_tab_contents_view_;

    // Used to render the sad tab. This will be non-NULL only when the sad tab is
    // visible.
    SadTabView* sad_tab_;

    // The id used in the ViewStorage to store the last focused view.
    int last_focused_view_storage_id_;

    // The context menu. Callbacks are asynchronous so we need to keep it around.
    //scoped_ptr<RenderViewContextMenuViews> context_menu_;

    // Set to true if we want to close the tab after the system drag operation
    // has finished.
    bool close_tab_after_drag_ends_;

    // Used to close the tab after the stack has unwound.
    base::OneShotTimer<TabContentsViewViews> close_tab_timer_;

    // The FocusManager associated with this tab.  Stored as it is not directly
    // accessible when un-parented.
    view::FocusManager* focus_manager_;

    DISALLOW_COPY_AND_ASSIGN(TabContentsViewViews);
};

#endif //__tab_contents_view_views_h__