
#ifndef __browser_view_h__
#define __browser_view_h__

#pragma once

#include <map>
#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/timer.h"

#include "ui_base/models/simple_menu_model.h"

#include "view/controls/menu/native_menu_win.h"
#include "view/controls/single_split_view.h"
#include "view/widget/widget_delegate.h"
#include "view/window/client_view.h"

#include "abstract_tab_strip_view.h"
#include "browser.h"
#include "browser_bubble_host.h"
#include "browser_frame.h"
#include "browser_window.h"
#include "infobar_container.h"
#include "tab_strip_model_observer.h"

// NOTE: For more information about the objects and files in this directory,
// view: http://dev.chromium.org/developers/design-documents/browser-window

namespace view
{
    class ExternalFocusTracker;
    class Menu;
}

class AccessiblePaneView;
class BookmarkBarView;
class Browser;
class BrowserBubble;
class ContentsContainer;
class FullscreenExitBubbleViews;
class InfoBarContainerView;
class TabContentsContainer;
class ToolbarView;

///////////////////////////////////////////////////////////////////////////////
// BrowserView
//
//  A ClientView subclass that provides the contents of a browser window,
//  including the TabStrip, toolbars, the content area etc.
//
class BrowserView : public BrowserBubbleHost,
    public BrowserWindow,
    public TabStripModelObserver,
    public ui::SimpleMenuModel::Delegate,
    public view::WidgetDelegate,
    public view::Widget::Observer,
    public view::ClientView,
    public InfoBarContainer::Delegate,
    public view::SingleSplitView::Observer
{
public:
    // The browser view's class name.
    static const char kViewClassName[];

    explicit BrowserView(Browser* browser);
    virtual ~BrowserView();

    void set_frame(BrowserFrame* frame) { frame_ = frame; }
    BrowserFrame* frame() const { return frame_; }

    // Returns a pointer to the BrowserView* interface implementation (an
    // instance of this object, typically) for a given native window, or NULL if
    // there is no such association.
    static BrowserView* GetBrowserViewForNativeWindow(HWND window);

    // Returns a Browser instance of this view.
    Browser* browser() const { return browser_.get(); }

    // Returns the apparent bounds of the toolbar, in BrowserView coordinates.
    // These differ from |toolbar_.bounds()| in that they match where the toolbar
    // background image is drawn -- slightly outside the "true" bounds
    // horizontally, and, when using vertical tabs, behind the tab column. Note
    // that this returns the bounds for the toolbar area, which could just be the
    // spacer bounds if in compact navigation mode.
    virtual gfx::Rect GetToolbarBounds() const;

    // Returns the bounds of the content area, in the coordinates of the
    // BrowserView's parent.
    gfx::Rect GetClientAreaBounds() const;

    // Returns the preferred height of the TabStrip. Used to position the OTR
    // avatar icon.
    virtual int GetTabStripHeight() const;

    // Takes some view's origin (relative to this BrowserView) and offsets it such
    // that it can be used as the source origin for seamlessly tiling the toolbar
    // background image over that view.
    gfx::Point OffsetPointForToolbarBackgroundImage(
        const gfx::Point& point) const;

    // Returns the width of the currently displayed sidebar or 0.
    int GetSidebarWidth() const;

    // Accessor for the TabStrip.
    AbstractTabStripView* tabstrip() const { return tabstrip_; }

    // Accessor for the Toolbar.
    ToolbarView* toolbar() const { return toolbar_; }

    // Returns true if various window components are visible.
    virtual bool IsTabStripVisible() const;

    // Handle the specified |accelerator| being pressed.
    virtual bool AcceleratorPressed(const view::Accelerator& accelerator);

    // Provides the containing frame with the accelerator for the specified
    // command id. This can be used to provide menu item shortcut hints etc.
    // Returns true if an accelerator was found for the specified |cmd_id|, false
    // otherwise.
    bool GetAccelerator(int cmd_id, ui::Accelerator* accelerator);

    // Shows the next app-modal dialog box, if there is one to be shown, or moves
    // an existing showing one to the front. Returns true if one was shown or
    // activated, false if none was shown.
    bool ActivateAppModalDialog() const;

    // Returns the selected TabContents[Wrapper]. Used by our NonClientView's
    // TabIconView::TabContentsProvider implementations.
    // TODO(beng): exposing this here is a bit bogus, since it's only used to
    // determine loading state. It'd be nicer if we could change this to be
    // bool IsSelectedTabLoading() const; or something like that. We could even
    // move it to a WindowDelegate subclass.
    TabContents* GetSelectedTabContents() const;
    TabContentsWrapper* GetSelectedTabContentsWrapper() const;

    // Called right before displaying the system menu to allow the BrowserView
    // to add or delete entries.
    void PrepareToRunSystemMenu(HMENU menu);

    // Returns true if the Browser object associated with this BrowserView is a
    // tabbed-type window (i.e. a browser window, not an app or popup).
    bool IsBrowserTypeNormal() const
    {
        return browser_->is_type_tabbed();
    }

    // Returns true if the Browser object associated with this BrowserView is a
    // panel window.
    bool IsBrowserTypePanel() const
    {
        return browser_->is_type_panel();
    }

    // Returns true if the Browser object associated with this BrowserView is a
    // popup window.
    bool IsBrowserTypePopup() const
    {
        return browser_->is_type_popup();
    }

    // Returns true if the specified point(BrowserView coordinates) is in
    // in the window caption area of the browser window.
    bool IsPositionInWindowCaption(const gfx::Point& point);

    // Returns whether the fullscreen bubble is visible or not.
    virtual bool IsFullscreenBubbleVisible() const;

    // Invoked from the frame when the full screen state changes. This is only
    // used on Linux.
    void FullScreenStateChanged();

    // Restores the focused view. This is also used to set the initial focus
    // when a new browser window is created.
    void RestoreFocus();

    // Overridden from BrowserWindow:
    virtual void Show();
    virtual void ShowInactive();
    virtual void SetBounds(const gfx::Rect& bounds);
    virtual void Close();
    virtual void Activate();
    virtual void Deactivate();
    virtual bool IsActive() const;
    virtual void FlashFrame();
    virtual HWND GetNativeHandle();
    virtual void ToolbarSizeChanged(bool is_animating);
    virtual void UpdateTitleBar();
    virtual void BookmarkBarStateChanged(
        BookmarkBar::AnimateChangeType change_type);
    virtual void UpdateLoadingAnimations(bool should_animate);
    virtual void SetStarredState(bool is_starred);
    virtual gfx::Rect GetRestoredBounds() const;
    virtual gfx::Rect GetBounds() const;
    virtual bool IsMaximized() const;
    virtual bool IsMinimized() const;
    virtual void SetFullscreen(bool fullscreen);
    virtual bool IsFullscreen() const;
    virtual LocationBar* GetLocationBar() const;
    virtual void SetFocusToLocationBar(bool select_all);
    virtual void UpdateReloadStopState(bool is_loading, bool force);
    virtual void UpdateToolbar(TabContentsWrapper* contents,
        bool should_restore_state);
    virtual void FocusToolbar();
    virtual void FocusAppMenu();
    virtual void FocusBookmarksToolbar();
    virtual void FocusChromeOSStatus() {}
    virtual void RotatePaneFocus(bool forwards);
    virtual void DestroyBrowser();
    virtual bool IsBookmarkBarVisible() const;
    virtual bool IsBookmarkBarAnimating() const;
    virtual bool IsTabStripEditable() const;
    virtual bool IsToolbarVisible() const;
    virtual void DisableInactiveFrame();
    virtual void ToggleBookmarkBar();
    virtual void ShowAboutChromeDialog();
    virtual void ShowUpdateChromeDialog();
    virtual void ShowTaskManager();
    virtual void ShowBackgroundPages();
    virtual void ShowBookmarkBubble(const Url& url, bool already_bookmarked);
    virtual void ShowRepostFormWarningDialog(TabContents* tab_contents);
    virtual void ShowCollectedCookiesDialog(TabContents* tab_contents);
    virtual void UserChangedTheme();
    virtual int GetExtraRenderViewHeight() const;
    virtual void TabContentsFocused(TabContents* source);
    virtual void ShowAppMenu();
    virtual void Cut();
    virtual void Copy();
    virtual void Paste();
    virtual void ToggleTabStripMode();
    virtual WindowOpenDisposition GetDispositionForPopupBounds(
        const gfx::Rect& bounds);

    // Overridden from BrowserWindowTesting:
    virtual LocationBarView* GetLocationBarView() const;

    // Overridden from TabStripModelObserver:
    virtual void TabDetachedAt(TabContentsWrapper* contents, int index);
    virtual void TabDeactivated(TabContentsWrapper* contents);
    virtual void ActiveTabChanged(TabContentsWrapper* old_contents,
        TabContentsWrapper* new_contents,
        int index,
        bool user_gesture);
    virtual void TabReplacedAt(TabStripModel* tab_strip_model,
        TabContentsWrapper* old_contents,
        TabContentsWrapper* new_contents,
        int index);
    virtual void TabStripEmpty();

    // Overridden from ui::SimpleMenuModel::Delegate:
    virtual bool IsCommandIdChecked(int command_id) const;
    virtual bool IsCommandIdEnabled(int command_id) const;
    virtual bool GetAcceleratorForCommandId(
        int command_id, ui::Accelerator* accelerator);
    virtual bool IsItemForCommandIdDynamic(int command_id) const;
    virtual string16 GetLabelForCommandId(int command_id) const;
    virtual void ExecuteCommand(int command_id);

    // Overridden from view::WidgetDelegate:
    virtual bool CanResize() const OVERRIDE;
    virtual bool CanMaximize() const OVERRIDE;
    virtual bool CanActivate() const OVERRIDE;
    virtual bool IsModal() const OVERRIDE;
    virtual std::wstring GetWindowTitle() const OVERRIDE;
    virtual std::wstring GetAccessibleWindowTitle() const OVERRIDE;
    virtual view::View* GetInitiallyFocusedView() OVERRIDE;
    virtual bool ShouldShowWindowTitle() const OVERRIDE;
    virtual SkBitmap GetWindowAppIcon() OVERRIDE;
    virtual SkBitmap GetWindowIcon() OVERRIDE;
    virtual bool ShouldShowWindowIcon() const OVERRIDE;
    virtual bool ExecuteWindowsCommand(int command_id) OVERRIDE;
    virtual std::wstring GetWindowName() const OVERRIDE;
    virtual void SaveWindowPlacement(const gfx::Rect& bounds,
        ui::WindowShowState show_state) OVERRIDE;
    virtual bool GetSavedWindowPlacement(
        gfx::Rect* bounds,
        ui::WindowShowState* show_state) const OVERRIDE;
    virtual view::View* GetContentsView() OVERRIDE;
    virtual view::ClientView* CreateClientView(view::Widget* widget) OVERRIDE;
    virtual void OnWindowBeginUserBoundsChange() OVERRIDE;
    virtual void OnWidgetMove() OVERRIDE;
    virtual view::Widget* GetWidget() OVERRIDE;
    virtual const view::Widget* GetWidget() const OVERRIDE;

    // Overridden from view::Widget::Observer
    virtual void OnWidgetActivationChanged(view::Widget* widget,
        bool active) OVERRIDE;

    // Overridden from view::ClientView:
    virtual bool CanClose() OVERRIDE;
    virtual int NonClientHitTest(const gfx::Point& point) OVERRIDE;
    virtual gfx::Size GetMinimumSize() OVERRIDE;

    // InfoBarContainer::Delegate overrides
    virtual SkColor GetInfoBarSeparatorColor() const OVERRIDE;
    virtual void InfoBarContainerStateChanged(bool is_animating) OVERRIDE;
    virtual bool DrawInfoBarArrows(int* x) const OVERRIDE;

    // view::SingleSplitView::Observer overrides:
    virtual bool SplitHandleMoved(view::SingleSplitView* view) OVERRIDE;

protected:
    // Appends to |toolbars| a pointer to each AccessiblePaneView that
    // can be traversed using F6, in the order they should be traversed.
    // Abstracted here so that it can be extended for Chrome OS.
    virtual void GetAccessiblePanes(
        std::vector<AccessiblePaneView*>* panes);

    // Save the current focused view to view storage
    void SaveFocusedView();

    int last_focused_view_storage_id() const
    {
        return last_focused_view_storage_id_;
    }

    // Overridden from view::View:
    virtual std::string GetClassName() const OVERRIDE;
    virtual void Layout() OVERRIDE;
    virtual void PaintChildren(gfx::Canvas* canvas) OVERRIDE;
    virtual void ViewHierarchyChanged(bool is_add,
        view::View* parent,
        view::View* child) OVERRIDE;
    virtual void ChildPreferredSizeChanged(View* child) OVERRIDE;
    virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

    // Factory Method.
    // Returns a new LayoutManager for this browser view. A subclass may
    // override to implement different layout policy.
    virtual view::LayoutManager* CreateLayoutManager() const;

    // Initializes a new TabStrip for the browser view. This can be performed
    // multiple times over the life of the browser, and is run when the display
    // mode for the tabstrip changes from horizontal to vertical.
    virtual void InitTabStrip(TabStripModel* tab_strip_model);

    // Factory Method.
    // Returns a new ToolbarView for this browser view. A subclass may
    // override to implement different layout policy.
    virtual ToolbarView* CreateToolbar() const;

    // Browser window related initializations.
    virtual void Init();

    // Callback for the loading animation(s) associated with this view.
    virtual void LoadingAnimationCallback();

private:
    friend class BrowserViewLayout;

    // Creates the system menu.
    void InitSystemMenu();

    // Returns the BrowserViewLayout.
    BrowserViewLayout* GetBrowserViewLayout() const;

    // Prepare to show the Bookmark Bar for the specified TabContents. Returns
    // true if the Bookmark Bar can be shown (i.e. it's supported for this
    // Browser type) and there should be a subsequent re-layout to show it.
    // |contents| can be NULL.
    bool MaybeShowBookmarkBar(TabContentsWrapper* contents);

    // Prepare to show an Info Bar for the specified TabContents. Returns true
    // if there is an Info Bar to show and one is supported for this Browser
    // type, and there should be a subsequent re-layout to show it.
    // |contents| can be NULL.
    bool MaybeShowInfoBar(TabContentsWrapper* contents);

    // Updates sidebar UI according to the current tab and sidebar state.
    void UpdateSidebar();
    // Displays active sidebar linked to the |tab_contents| or hides sidebar UI,
    // if there's no such sidebar.
    void UpdateSidebarForContents(TabContentsWrapper* tab_contents);

    // Updates various optional child Views, e.g. Bookmarks Bar, Info Bar or the
    // Download Shelf in response to a change notification from the specified
    // |contents|. |contents| can be NULL. In this case, all optional UI will be
    // removed.
    void UpdateUIForContents(TabContentsWrapper* contents);

    // Updates an optional child View, e.g. Bookmarks Bar, Info Bar, Download
    // Shelf. If |*old_view| differs from new_view, the old_view is removed and
    // the new_view is added. This is intended to be used when swapping in/out
    // child views that are referenced via a field.
    // Returns true if anything was changed, and a re-Layout is now required.
    bool UpdateChildViewAndLayout(view::View* new_view, view::View** old_view);

    // Invoked to update the necessary things when our fullscreen state changes
    // to |fullscreen|. On Windows this is invoked immediately when we toggle the
    // full screen state. On Linux changing the fullscreen state is async, so we
    // ask the window to change it's fullscreen state, then when we get
    // notification that it succeeded this method is invoked.
    void ProcessFullscreen(bool fullscreen);

    // Copy the accelerator table from the app resources into something we can
    // use.
    void LoadAccelerators();

    // Builds the correct menu for when we have minimal chrome.
    void BuildSystemMenuForBrowserWindow();
    void BuildSystemMenuForAppOrPopupWindow();

    // Retrieves the command id for the specified Windows app command.
    int GetCommandIDForAppCommandID(int app_command_id) const;

    // Invoked from ActiveTabChanged or when instant is made active.  If
    // |change_tab_contents| is true, |new_contents| is added to the view
    // hierarchy, if |change_tab_contents| is false, it's assumed |new_contents|
    // has already been added to the view hierarchy.
    void ProcessTabSelected(TabContentsWrapper* new_contents,
        bool change_tab_contents);

    // Exposes resize corner size to BrowserViewLayout.
    gfx::Size GetResizeCornerSize() const;

    // Shows the about chrome modal dialog and returns the Window object.
    view::Widget* DoShowAboutChromeDialog();

    // Set the value of |toolbar_| and hook it into the views hierarchy
    void SetToolbar(ToolbarView* toolbar);

    // Last focused view that issued a tab traversal.
    int last_focused_view_storage_id_;

    // The BrowserFrame that hosts this view.
    BrowserFrame* frame_;

    // The Browser object we are associated with.
    scoped_ptr<Browser> browser_;

    // BrowserView layout (LTR one is pictured here).
    //
    // --------------------------------------------------------------------------
    // |         | Tabs (1)                                                     |
    // |         |--------------------------------------------------------------|
    // |         | Navigation buttons, menus and the address bar (toolbar_)     |
    // |         |--------------------------------------------------------------|
    // |         | All infobars (infobar_container_) *                          |
    // |         |--------------------------------------------------------------|
    // |         | Bookmarks (bookmark_bar_view_) *                             |
    // |         |--------------------------------------------------------------|
    // |         |Page content (contents_)              ||                      |
    // |         |--------------------------------------|| Sidebar content      |
    // |         || contents_container_ and/or         ||| (sidebar_container_) |
    // |         || preview_container_                 |||                      |
    // |         ||                                    |(3)                     |
    // | Tabs (2)||                                    |||                      |
    // |         ||                                    |||                      |
    // |         ||                                    |||                      |
    // |         ||                                    |||                      |
    // |         |--------------------------------------||                      |
    // --------------------------------------------------------------------------
    //
    // (1) - tabstrip_, default position
    // (2) - tabstrip_, position when side tabs are enabled
    // (3) - sidebar_split_
    //
    // * - The bookmark bar and info bar are swapped when on the new tab page.
    //     Additionally contents_ is positioned on top of the bookmark bar when
    //     the bookmark bar is detached. This is done to allow the
    //     preview_container_ to appear over the bookmark bar.

    // Tool/Info bars that we are currently showing. Used for layout.
    // active_bookmark_bar_ is either NULL, if the bookmark bar isn't showing,
    // or is bookmark_bar_view_ if the bookmark bar is showing.
    view::View* active_bookmark_bar_;

    // The TabStrip.
    AbstractTabStripView* tabstrip_;

    // The Toolbar containing the navigation buttons, menus and the address bar.
    ToolbarView* toolbar_;

    // The Bookmark Bar View for this window. Lazily created.
    scoped_ptr<BookmarkBarView> bookmark_bar_view_;

    // The InfoBarContainerView that contains InfoBars for the current tab.
    InfoBarContainerView* infobar_container_;

    // The view that contains sidebar for the current tab.
    TabContentsContainer* sidebar_container_;

    // Split view containing the contents container and sidebar container.
    view::SingleSplitView* sidebar_split_;

    // The view that contains the selected TabContents.
    TabContentsContainer* contents_container_;

    // The view that contains instant's TabContents.
    TabContentsContainer* preview_container_;

    // The view managing both the contents_container_ and preview_container_.
    ContentsContainer* contents_;

    // A mapping between accelerators and commands.
    std::map<view::Accelerator, int> accelerator_table_;

    // True if we have already been initialized.
    bool initialized_;

    // True if we should ignore requests to layout.  This is set while toggling
    // fullscreen mode on and off to reduce jankiness.
    bool ignore_layout_;

    //scoped_ptr<FullscreenExitBubbleViews> fullscreen_bubble_;

    // The additional items we insert into the system menu.
    scoped_ptr<view::SystemMenuModel> system_menu_contents_;
    //scoped_ptr<ZoomMenuModel> zoom_menu_contents_;
    //scoped_ptr<EncodingMenuModel> encoding_menu_contents_;
    // The wrapped system menu itself.
    scoped_ptr<view::NativeMenuWin> system_menu_;

    // The timer used to update frames for the Loading Animation.
    base::RepeatingTimer<BrowserView> loading_animation_timer_;

    // Used to measure the loading spinner animation rate.
    base::TimeTicks last_animation_time_;

    DISALLOW_COPY_AND_ASSIGN(BrowserView);
};

#endif //__browser_view_h__