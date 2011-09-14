
#ifndef __tab_contents_view_h__
#define __tab_contents_view_h__

#pragma once

#include <windows.h>

#include <string>

#include "base/basic_types.h"
#include "base/process_util.h"

#include "ui_gfx/rect.h"
#include "ui_gfx/size.h"

class RenderViewHost;
class RenderWidgetHost;
class RenderWidgetHostView;
class TabContents;

// The TabContentsView is an interface that is implemented by the platform-
// dependent web contents views. The TabContents uses this interface to talk to
// them. View-related messages will also get forwarded directly to this class
// from RenderViewHost via RenderViewHostDelegate::View.
class TabContentsView
{
public:
    virtual ~TabContentsView();

    virtual void CreateView(const gfx::Size& initial_size) = 0;

    // Sets up the View that holds the rendered web page, receives messages for
    // it and contains page plugins. The host view should be sized to the current
    // size of the TabContents.
    virtual RenderWidgetHostView* CreateViewForWidget(
        RenderWidgetHost* render_widget_host) = 0;

    // Returns the native widget that contains the contents of the tab.
    virtual HWND GetNativeView() const = 0;

    // Returns the native widget with the main content of the tab (i.e. the main
    // render view host, though there may be many popups in the tab as children of
    // the container).
    virtual HWND GetContentNativeView() const = 0;

    // Returns the outermost native view. This will be used as the parent for
    // dialog boxes.
    virtual HWND GetTopLevelNativeWindow() const = 0;

    // Computes the rectangle for the native widget that contains the contents of
    // the tab relative to its parent.
    virtual void GetContainerBounds(gfx::Rect* out) const = 0;

    // Helper function for GetContainerBounds. Most callers just want to know the
    // size, and this makes it more clear.
    gfx::Size GetContainerSize() const
    {
        gfx::Rect rc;
        GetContainerBounds(&rc);
        return gfx::Size(rc.width(), rc.height());
    }

    // Sets the page title for the native widgets corresponding to the view. This
    // is not strictly necessary and isn't expected to be displayed anywhere, but
    // can aid certain debugging tools such as Spy++ on Windows where you are
    // trying to find a specific window.
    virtual void SetPageTitle(const std::wstring& title) = 0;

    // Used to notify the view that a tab has crashed so each platform can
    // prepare the sad tab.
    virtual void OnTabCrashed(base::TerminationStatus status,
        int error_code) = 0;

    // TODO(brettw) this is a hack. It's used in two places at the time of this
    // writing: (1) when render view hosts switch, we need to size the replaced
    // one to be correct, since it wouldn't have known about sizes that happened
    // while it was hidden; (2) in constrained windows.
    //
    // (1) will be fixed once interstitials are cleaned up. (2) seems like it
    // should be cleaned up or done some other way, since this works for normal
    // TabContents without the special code.
    virtual void SizeContents(const gfx::Size& size) = 0;

    // Invoked when the TabContents is notified that the RenderView has been
    // fully created.
    virtual void RenderViewCreated(RenderViewHost* host) = 0;

    // Sets focus to the native widget for this tab.
    virtual void Focus() = 0;

    // Sets focus to the appropriate element when the tab contents is shown the
    // first time.
    virtual void SetInitialFocus() = 0;

    // Stores the currently focused view.
    virtual void StoreFocus() = 0;

    // Restores focus to the last focus view. If StoreFocus has not yet been
    // invoked, SetInitialFocus is invoked.
    virtual void RestoreFocus() = 0;

    // Notification that the preferred size of the contents has changed.
    virtual void UpdatePreferredSize(const gfx::Size& pref_size) = 0;

    // If we try to close the tab while a drag is in progress, we crash.  These
    // methods allow the tab contents to determine if a drag is in progress and
    // postpone the tab closing.
    virtual bool IsDoingDrag() const = 0;
    virtual void CancelDragAndCloseTab() = 0;

    // If we close the tab while a UI control is in an event-tracking
    // loop, the control may message freed objects and crash.
    // TabContents::Close() calls IsEventTracking(), and if it returns
    // true CloseTabAfterEventTracking() is called and the close is not
    // completed.
    virtual bool IsEventTracking() const = 0;
    virtual void CloseTabAfterEventTracking() = 0;

    // Get the bounds of the View, relative to the parent.
    // TODO(beng): Return a rect rather than using an out param.
    virtual void GetViewBounds(gfx::Rect* out) const = 0;

protected:
    TabContentsView(); // Abstract interface.

private:
    DISALLOW_COPY_AND_ASSIGN(TabContentsView);
};

#endif //__tab_contents_view_h__