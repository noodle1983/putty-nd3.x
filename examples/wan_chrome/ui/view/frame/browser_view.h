
#ifndef __wan_chrome_ui_view_frame_browser_view_h__
#define __wan_chrome_ui_view_frame_browser_view_h__

#pragma once

#include "message/timer.h"

#include "view_framework/controls/single_split_view.h"
#include "view_framework/controls/menu/simple_menu_model.h"
#include "view_framework/controls/menu/native_menu_win.h"
#include "view_framework/view/client_view.h"
#include "view_framework/window/window_delegate.h"

#include "../../browser.h"
#include "../../browser_window.h"
#include "browser_frame.h"

///////////////////////////////////////////////////////////////////////////////
// BrowserView
//
//  A ClientView subclass that provides the contents of a browser window,
//  including the TabStrip, toolbars, download shelves, the content area etc.
class BrowserView : public BrowserWindow,
    public NotificationObserver,
    public view::SimpleMenuModel::Delegate,
    public view::WindowDelegate,
    public view::ClientView,
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

    // Returns the bounds of the content area, in the coordinates of the
    // BrowserView's parent.
    gfx::Rect GetClientAreaBounds() const;

    // Takes some view's origin (relative to this BrowserView) and offsets it such
    // that it can be used as the source origin for seamlessly tiling the toolbar
    // background image over that view.
    gfx::Point OffsetPointForToolbarBackgroundImage(
        const gfx::Point& point) const;

    // Returns true if various window components are visible.
    bool IsTabStripVisible() const;

    // Handle the specified |accelerator| being pressed.
    virtual bool AcceleratorPressed(const view::Accelerator& accelerator);

    // Provides the containing frame with the accelerator for the specified
    // command id. This can be used to provide menu item shortcut hints etc.
    // Returns true if an accelerator was found for the specified |cmd_id|, false
    // otherwise.
    bool GetAccelerator(int cmd_id, view::MenuAccelerator* accelerator);

    // Called right before displaying the system menu to allow the BrowserView
    // to add or delete entries.
    void PrepareToRunSystemMenu(HMENU menu);

    // Returns true if the Browser object associated with this BrowserView is a
    // normal-type window (i.e. a browser window, not an app or popup).
    bool IsBrowserTypeNormal() const
    {
        return browser_->type() == Browser::TYPE_NORMAL;
    }

    // Returns true if the Browser object associated with this BrowserView is a
    // app panel window.
    bool IsBrowserTypePanel() const
    {
        return browser_->type() == Browser::TYPE_APP_PANEL;
    }

    // Returns true if the Browser object associated with this BrowserView is a
    // popup window.
    bool IsBrowserTypePopup() const
    {
        return (browser_->type() & Browser::TYPE_POPUP) != 0;
    }

    // Returns true if the specified point(BrowserView coordinates) is in
    // in the window caption area of the browser window.
    bool IsPositionInWindowCaption(const gfx::Point& point);

    // Invoked from the frame when the full screen state changes. This is only
    // used on Linux.
    void FullScreenStateChanged();

    // Restores the focused view. This is also used to set the initial focus
    // when a new browser window is created.
    void RestoreFocus();

    // Overridden from BrowserWindow:
    virtual void Show();
    virtual void SetBounds(const gfx::Rect& bounds);
    virtual void Close();
    virtual void Activate();
    virtual void Deactivate();
    virtual bool IsActive() const;
    virtual void FlashFrame();
    virtual HWND GetNativeHandle();
    virtual void UpdateTitleBar();
    virtual gfx::Rect GetRestoredBounds() const;
    virtual gfx::Rect GetBounds() const;
    virtual bool IsMaximized() const;
    virtual void SetFullscreen(bool fullscreen);
    virtual bool IsFullscreen() const;
    virtual bool IsFullscreenBubbleVisible() const;
    virtual void FocusAppMenu();
    virtual void RotatePaneFocus(bool forwards);
    virtual bool IsToolbarVisible() const;
    virtual void DisableInactiveFrame();
    virtual void UserChangedTheme();
    virtual void DestroyBrowser();

    // Overridden from NotificationObserver:
    virtual void Observe(NotificationType type,
        const NotificationSource& source,
        const NotificationDetails& details);

    // Overridden from view::SimpleMenuModel::Delegate:
    virtual bool IsCommandIdChecked(int command_id) const;
    virtual bool IsCommandIdEnabled(int command_id) const;
    virtual bool GetAcceleratorForCommandId(
        int command_id, view::MenuAccelerator* accelerator);
    virtual bool IsItemForCommandIdDynamic(int command_id) const;
    virtual string16 GetLabelForCommandId(int command_id) const;
    virtual void ExecuteCommand(int command_id);

    // Overridden from view::WindowDelegate:
    virtual bool CanResize() const;
    virtual bool CanMaximize() const;
    virtual bool CanActivate() const;
    virtual bool IsModal() const;
    virtual std::wstring GetWindowTitle() const;
    virtual std::wstring GetAccessibleWindowTitle() const;
    virtual view::View* GetInitiallyFocusedView();
    virtual bool ShouldShowWindowTitle() const;
    virtual SkBitmap GetWindowAppIcon();
    virtual SkBitmap GetWindowIcon();
    virtual bool ShouldShowWindowIcon() const;
    virtual bool ExecuteWindowsCommand(int command_id);
    virtual std::wstring GetWindowName() const;
    virtual void SaveWindowPlacement(const gfx::Rect& bounds,
        bool maximized);
    virtual bool GetSavedWindowBounds(gfx::Rect* bounds) const;
    virtual bool GetSavedMaximizedState(bool* maximized) const;
    virtual view::View* GetContentsView();
    virtual view::ClientView* CreateClientView(view::Window* window);
    virtual void OnWindowActivationChanged(bool active);
    virtual void OnWindowBeginUserBoundsChange();
    virtual void OnWidgetMove();

    // Overridden from view::ClientView:
    virtual bool CanClose();
    virtual int NonClientHitTest(const gfx::Point& point);
    virtual gfx::Size GetMinimumSize();

    // view::SingleSplitView::Observer overrides:
    virtual bool SplitHandleMoved(view::SingleSplitView* view);

protected:
    // Save the current focused view to view storage
    void SaveFocusedView();

    int last_focused_view_storage_id() const
    {
        return last_focused_view_storage_id_;
    }

    // Overridden from view::View:
    virtual std::string GetClassName() const;
    virtual void Layout();
    virtual void PaintChildren(gfx::Canvas* canvas);
    virtual void ViewHierarchyChanged(bool is_add,
        view::View* parent,
        view::View* child);
    virtual void ChildPreferredSizeChanged(View* child);
    virtual void GetAccessibleState(AccessibleViewState* state);

    // Factory Methods.
    // Returns a new LayoutManager for this browser view. A subclass may
    // override to implemnet different layout pocily.
    virtual view::LayoutManager* CreateLayoutManager() const;

    // Browser window related initializations.
    virtual void Init();

private:
    friend class BrowserViewLayout;

    // Creates the system menu.
    void InitSystemMenu();

    // Get the X value, in this BrowserView's coordinate system, where
    // the points of the infobar arrows should be anchored.  This is the
    // center of the omnibox location icon.
    int GetInfoBarArrowCenterX() const;

    // Returns the BrowserViewLayout.
    BrowserViewLayout* GetBrowserViewLayout() const;

    // Layout the Status Bubble.
    void LayoutStatusBubble();

    // Updates an optional child View, e.g. Bookmarks Bar, Info Bar, Download
    // Shelf. If |*old_view| differs from new_view, the old_view is removed and
    // the new_view is added. This is intended to be used when swapping in/out
    // child view that are referenced via a field.
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
    void BuildSystemMenuForAppOrPopupWindow(bool is_app);

    // Retrieves the command id for the specified Windows app command.
    int GetCommandIDForAppCommandID(int app_command_id) const;

    // Callback for the loading animation(s) associated with this view.
    void LoadingAnimationCallback();

    // Possibly records a user metrics action corresponding to the passed-in
    // accelerator.  Only implemented for Chrome OS, where we're interested in
    // learning about how frequently the top-row keys are used.
    void UpdateAcceleratorMetrics(const view::Accelerator& accelerator,
        int command_id);

    // Exposes resize corner size to BrowserViewLayout.
    gfx::Size GetResizeCornerSize() const;

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
    // |         |==(4)=========================================================|
    // |         |                                                              |
    // |         |                                                              |
    // |         | Debugger (devtools_container_)                               |
    // |         |                                                              |
    // |         |                                                              |
    // |         |--------------------------------------------------------------|
    // |         | Active downloads (download_shelf_)                           |
    // --------------------------------------------------------------------------
    //
    // (1) - tabstrip_, default position
    // (2) - tabstrip_, position when side tabs are enabled
    // (3) - sidebar_split_
    // (4) - contents_split_
    //
    // * - The bookmark bar and info bar are swapped when on the new tab page.
    //     Additionally contents_ is positioned on top of the bookmark bar when
    //     the bookmark bar is detached. This is done to allow the
    //     preview_container_ to appear over the bookmark bar.

    // A mapping between accelerators and commands.
    std::map<view::Accelerator, int> accelerator_table_;

    // True if we have already been initialized.
    bool initialized_;

    // True if we should ignore requests to layout.  This is set while toggling
    // fullscreen mode on and off to reduce jankiness.
    bool ignore_layout_;

    // The additional items we insert into the system menu.
    scoped_ptr<view::SystemMenuModel> system_menu_contents_;
    // The wrapped system menu itself.
    scoped_ptr<view::NativeMenuWin> system_menu_;

    // The timer used to update frames for the Loading Animation.
    base::RepeatingTimer<BrowserView> loading_animation_timer_;

    NotificationRegistrar registrar_;

    // Used to measure the loading spinner animation rate.
    base::TimeTicks last_animation_time_;

    DISALLOW_COPY_AND_ASSIGN(BrowserView);
};

#endif //__wan_chrome_ui_view_frame_browser_view_h__