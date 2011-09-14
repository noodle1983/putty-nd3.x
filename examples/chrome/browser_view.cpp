
#include "browser_view.h"

#include "base/auto_reset.h"
#include "base/i18n/rtl.h"
#include "base/metric/histogram.h"
#include "base/utf_string_conversions.h"

#include "ui_gfx/canvas.h"
#include "ui_gfx/scrollbar_size.h"

#include "ui_base/accessibility/accessible_view_state.h"
#include "ui_base/l10n/l10n_util.h"
#include "ui_base/resource/resource_bundle.h"
#include "ui_base/view_prop.h"

#include "view/focus/view_storage.h"
#include "view/layout/grid_layout.h"

#include "../wanui_res/resource.h"

#include "browser.h"
#include "browser_list.h"
#include "browser_view_layout.h"
#include "bookmark_bar_view.h"
#include "chrome_command_ids.h"
#include "contents_container.h"
#include "detachable_toolbar_view.h"
#include "infobar_container_view.h"
#include "location_bar_view.h"
#include "location_icon_view.h"
#include "tab_contents.h"
#include "tab_contents_view.h"
#include "tab_contents_container_views.h"
#include "tab_contents_wrapper.h"
#include "tab_strip_factory.h"
#include "tab_strip_model.h"
#include "toolbar_view.h"

using base::TimeDelta;
using view::ColumnSet;
using view::GridLayout;

// The name of a key to store on the window handle so that other code can
// locate this object using just the handle.
static const char* const kBrowserViewKey = "__BROWSER_VIEW__";
// How frequently we check for hung plugin windows.
static const int kDefaultHungPluginDetectFrequency = 2000;

// How long do we wait before we consider a window hung (in ms).
static const int kDefaultPluginMessageResponseTimeout = 30000;
// The number of milliseconds between loading animation frames.
static const int kLoadingAnimationFrameTimeMs = 30;
// The amount of space we expect the window border to take up.
static const int kWindowBorderWidth = 5;

// How round the 'new tab' style bookmarks bar is.
static const int kNewtabBarRoundness = 5;

// Returned from BrowserView::GetClassName.
const char BrowserView::kViewClassName[] = "browser/BrowserView";

///////////////////////////////////////////////////////////////////////////////
// BookmarkExtensionBackground, private:
// This object serves as the view::Background object which is used to layout
// and paint the bookmark bar.
class BookmarkExtensionBackground : public view::Background
{
public:
    BookmarkExtensionBackground(BrowserView* browser_view,
        DetachableToolbarView* host_view,
        Browser* browser);

    // View methods overridden from views:Background.
    virtual void Paint(gfx::Canvas* canvas, view::View* view) const;

private:
    BrowserView* browser_view_;

    // The view hosting this background.
    DetachableToolbarView* host_view_;

    Browser* browser_;

    DISALLOW_COPY_AND_ASSIGN(BookmarkExtensionBackground);
};

BookmarkExtensionBackground::BookmarkExtensionBackground(
    BrowserView* browser_view,
    DetachableToolbarView* host_view,
    Browser* browser)
    : browser_view_(browser_view),
    host_view_(host_view),
    browser_(browser) {}

void BookmarkExtensionBackground::Paint(gfx::Canvas* canvas,
                                        view::View* view) const
{
    ui::ThemeProvider* tp = host_view_->GetThemeProvider();
    int toolbar_overlap = host_view_->GetToolbarOverlap();
    // The client edge is drawn below the toolbar bounds.
    if(toolbar_overlap)
    {
        toolbar_overlap += view::NonClientFrameView::kClientEdgeThickness;
    }
    if(host_view_->IsDetached())
    {
        // Draw the background to match the new tab page.
        int height = 0;
        TabContents* contents = browser_->GetSelectedTabContents();
        if(contents && contents->view())
        {
            height = contents->view()->GetContainerSize().height();
        }
        //NtpBackgroundUtil::PaintBackgroundDetachedMode(
        //    host_view_->GetThemeProvider(), canvas,
        //    gfx::Rect(0, toolbar_overlap, host_view_->width(),
        //    host_view_->height() - toolbar_overlap), height);

        // As 'hidden' according to the animation is the full in-tab state,
        // we invert the value - when current_state is at '0', we expect the
        // bar to be docked.
        double current_state = 1 - host_view_->GetAnimationValue();
        double h_padding =
            static_cast<double>(BookmarkBarView::kNewtabHorizontalPadding) *
            current_state;
        double v_padding =
            static_cast<double>(BookmarkBarView::kNewtabVerticalPadding) *
            current_state;

        SkRect rect;
        double roundness = 0;
        DetachableToolbarView::CalculateContentArea(current_state, h_padding,
            v_padding, &rect, &roundness, host_view_);
        DetachableToolbarView::PaintContentAreaBackground(canvas, tp, rect,
            roundness);
        DetachableToolbarView::PaintContentAreaBorder(canvas, tp, rect, roundness);
        if(!toolbar_overlap)
        {
            DetachableToolbarView::PaintHorizontalBorder(canvas, host_view_);
        }
    }
    else
    {
        DetachableToolbarView::PaintBackgroundAttachedMode(canvas, host_view_,
            browser_view_->OffsetPointForToolbarBackgroundImage(
            gfx::Point(host_view_->GetMirroredX(), host_view_->y())));
        if(host_view_->height() >= toolbar_overlap)
        {
            DetachableToolbarView::PaintHorizontalBorder(canvas, host_view_);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// ResizeCorner, private:

class ResizeCorner : public view::View
{
public:
    ResizeCorner()
    {
        EnableCanvasFlippingForRTLUI(true);
    }

    virtual void OnPaint(gfx::Canvas* canvas)
    {
        view::Widget* widget = GetWidget();
        if(!widget || (widget->IsMaximized() || widget->IsFullscreen()))
        {
            return;
        }

        //SkBitmap* bitmap = ui::ResourceBundle::GetSharedInstance().GetBitmapNamed(
        //    IDR_TEXTAREA_RESIZER);
        //bitmap->buildMipMap(false);
        //canvas->DrawBitmapInt(*bitmap, width() - bitmap->width(),
        //    height() - bitmap->height());
    }

    static gfx::Size GetSize()
    {
        // This is disabled until we find what makes us slower when we let
        // WebKit know that we have a resizer rect...
         int scrollbar_thickness = gfx::scrollbar_size();
         return gfx::Size(scrollbar_thickness, scrollbar_thickness);
    }

    virtual gfx::Size GetPreferredSize()
    {
        view::Widget* widget = GetWidget();
        return (!widget || widget->IsMaximized() || widget->IsFullscreen()) ?
            gfx::Size() : GetSize();
    }

    virtual void Layout()
    {
        if(parent())
        {
            gfx::Size ps = GetPreferredSize();
            // No need to handle Right to left text direction here,
            // our parent must take care of it for us...
            // TODO(alekseys): fix it.
            SetBounds(parent()->width() - ps.width(),
                parent()->height() - ps.height(), ps.width(), ps.height());
        }
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ResizeCorner);
};

///////////////////////////////////////////////////////////////////////////////
// BrowserView, public:

BrowserView::BrowserView(Browser* browser) : view::ClientView(NULL, NULL),
last_focused_view_storage_id_(view::ViewStorage::GetInstance()->CreateStorageID()),
frame_(NULL),
browser_(browser),
active_bookmark_bar_(NULL),
tabstrip_(NULL),
toolbar_(NULL),
infobar_container_(NULL),
sidebar_container_(NULL),
sidebar_split_(NULL),
contents_container_(NULL),
preview_container_(NULL),
contents_(NULL),
initialized_(false),
ignore_layout_(true)
{
    browser_->tabstrip_model()->AddObserver(this);
}

BrowserView::~BrowserView()
{
    browser_->tabstrip_model()->RemoveObserver(this);

    // The TabStrip attaches a listener to the model. Make sure we shut down the
    // TabStrip first so that it can cleanly remove the listener.
    if(tabstrip_)
    {
        tabstrip_->parent()->RemoveChildView(tabstrip_);
        delete tabstrip_;
        tabstrip_ = NULL;
    }
    // Child views maintain PrefMember attributes that point to
    // OffTheRecordProfile's PrefService which gets deleted by ~Browser.
    RemoveAllChildViews(true);
    // Explicitly set browser_ to NULL.
    browser_.reset();
}

// static
BrowserView* BrowserView::GetBrowserViewForNativeWindow(HWND window)
{
    if(IsWindow(window))
    {
        return reinterpret_cast<BrowserView*>(
            ui::ViewProp::GetValue(window, kBrowserViewKey));
    }
    return NULL;
}

gfx::Rect BrowserView::GetToolbarBounds() const
{
    gfx::Rect toolbar_bounds(toolbar_->bounds());
    if(toolbar_bounds.IsEmpty())
    {
        return toolbar_bounds;
    }
    // The apparent toolbar edges are outside the "real" toolbar edges.
    toolbar_bounds.Inset(-view::NonClientFrameView::kClientEdgeThickness, 0);
    return toolbar_bounds;
}

gfx::Rect BrowserView::GetClientAreaBounds() const
{
    gfx::Rect container_bounds = contents_->bounds();
    gfx::Point container_origin = container_bounds.origin();
    ConvertPointToView(this, parent(), &container_origin);
    container_bounds.set_origin(container_origin);
    return container_bounds;
}

int BrowserView::GetTabStripHeight() const
{
    // We want to return tabstrip_->height(), but we might be called in the midst
    // of layout, when that hasn't yet been updated to reflect the current state.
    // So return what the tabstrip height _ought_ to be right now.
    return IsTabStripVisible() ? tabstrip_->GetPreferredSize().height() : 0;
}

gfx::Point BrowserView::OffsetPointForToolbarBackgroundImage(
    const gfx::Point& point) const
{
    // The background image starts tiling horizontally at the window left edge and
    // vertically at the top edge of the horizontal tab strip (or where it would
    // be).  We expect our parent's origin to be the window origin.
    gfx::Point window_point(point.Add(GetMirroredPosition()));
    window_point.Offset(0, -frame_->GetHorizontalTabStripVerticalOffset(false));
    return window_point;
}

int BrowserView::GetSidebarWidth() const
{
    if(!sidebar_container_ || !sidebar_container_->IsVisible())
    {
        return 0;
    }
    return sidebar_split_->divider_offset();
}

bool BrowserView::IsTabStripVisible() const
{
    return browser_->SupportsWindowFeature(Browser::FEATURE_TABSTRIP);
}

bool BrowserView::AcceleratorPressed(const view::Accelerator& accelerator)
{
    std::map<view::Accelerator, int>::const_iterator iter =
        accelerator_table_.find(accelerator);
    DCHECK(iter != accelerator_table_.end());
    int command_id = iter->second;

    return browser_->ExecuteCommandIfEnabled(command_id);
}

bool BrowserView::GetAccelerator(int cmd_id, ui::Accelerator* accelerator)
{
    // The standard Ctrl-X, Ctrl-V and Ctrl-C are not defined as accelerators
    // anywhere so we need to check for them explicitly here.
    switch(cmd_id)
    {
    case IDC_CUT:
        *accelerator = view::Accelerator(ui::VKEY_X, false, true, false);
        return true;
    case IDC_COPY:
        *accelerator = view::Accelerator(ui::VKEY_C, false, true, false);
        return true;
    case IDC_PASTE:
        *accelerator = view::Accelerator(ui::VKEY_V, false, true, false);
        return true;
    }
    // Else, we retrieve the accelerator information from the accelerator table.
    std::map<view::Accelerator, int>::iterator it = accelerator_table_.begin();
    for(; it!=accelerator_table_.end(); ++it)
    {
        if(it->second == cmd_id)
        {
            *accelerator = it->first;
            return true;
        }
    }
    return false;
}

bool BrowserView::ActivateAppModalDialog() const
{
    // If another browser is app modal, flash and activate the modal browser.
    //if(AppModalDialogQueue::GetInstance()->HasActiveDialog())
    //{
    //    Browser* active_browser = BrowserList::GetLastActive();
    //    if(active_browser && (browser_ != active_browser))
    //    {
    //        active_browser->window()->FlashFrame();
    //        active_browser->window()->Activate();
    //    }
    //    AppModalDialogQueue::GetInstance()->ActivateModalDialog();
    //    return true;
    //}
    return false;
}

TabContents* BrowserView::GetSelectedTabContents() const
{
    return browser_->GetSelectedTabContents();
}

TabContentsWrapper* BrowserView::GetSelectedTabContentsWrapper() const
{
    return browser_->GetSelectedTabContentsWrapper();
}

void BrowserView::PrepareToRunSystemMenu(HMENU menu)
{
    system_menu_->UpdateStates();
}

bool BrowserView::IsPositionInWindowCaption(const gfx::Point& point)
{
    return GetBrowserViewLayout()->IsPositionInWindowCaption(point);
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, BrowserWindow implementation:

void BrowserView::Show()
{
    // The same fix as in BrowserWindowGtk::Show.
    //
    // The Browser must become the active browser when Show() is called.
    // But, on Gtk, the browser won't be shown until we return to the runloop.
    // Therefore we need to set the active window here explicitly. otherwise
    // any calls to BrowserList::GetLastActive() (for example, in bookmark_util),
    // will return the previous browser.
    BrowserList::SetLastActive(browser());

    // If the window is already visible, just activate it.
    if(frame_->IsVisible())
    {
        frame_->Activate();
        return;
    }

    // Setting the focus doesn't work when the window is invisible, so any focus
    // initialization that happened before this will be lost.
    //
    // We really "should" restore the focus whenever the window becomes unhidden,
    // but I think initializing is the only time where this can happen where
    // there is some focus change we need to pick up, and this is easier than
    // plumbing through an un-hide message all the way from the frame.
    //
    // If we do find there are cases where we need to restore the focus on show,
    // that should be added and this should be removed.
    RestoreFocus();

    frame_->Show();
}

void BrowserView::ShowInactive()
{
    if(!frame_->IsVisible())
    {
        frame_->ShowInactive();
    }
}

void BrowserView::SetBounds(const gfx::Rect& bounds)
{
    SetFullscreen(false);
    GetWidget()->SetBounds(bounds);
}

void BrowserView::Close()
{
    BrowserBubbleHost::Close();

    frame_->Close();
}

void BrowserView::Activate()
{
    frame_->Activate();
}

void BrowserView::Deactivate()
{
    frame_->Deactivate();
}

bool BrowserView::IsActive() const
{
    return frame_->IsActive();
}

void BrowserView::FlashFrame()
{
    FLASHWINFO fwi;
    fwi.cbSize = sizeof(fwi);
    fwi.hwnd = frame_->GetNativeWindow();
    fwi.dwFlags = FLASHW_ALL;
    fwi.uCount = 4;
    fwi.dwTimeout = 0;
    FlashWindowEx(&fwi);
}

HWND BrowserView::GetNativeHandle()
{
    return GetWidget()->GetTopLevelWidget()->GetNativeWindow();
}

namespace
{
    // Only used by ToolbarSizeChanged() below, but placed here because template
    // arguments (to AutoReset<>) must have external linkage.
    enum CallState { NORMAL, REENTRANT, REENTRANT_FORCE_FAST_RESIZE };
}

void BrowserView::ToolbarSizeChanged(bool is_animating)
{
    // The call to InfoBarContainer::SetMaxTopArrowHeight() below can result in
    // reentrancy; |call_state| tracks whether we're reentrant.  We can't just
    // early-return in this case because we need to layout again so the infobar
    // container's bounds are set correctly.
    static CallState call_state = NORMAL;

    // A reentrant call can (and should) use the fast resize path unless both it
    // and the normal call are both non-animating.
    bool use_fast_resize = is_animating ||
        (call_state == REENTRANT_FORCE_FAST_RESIZE);
    if(use_fast_resize)
    {
        contents_container_->SetFastResize(true);
    }
    UpdateUIForContents(browser_->GetSelectedTabContentsWrapper());
    if(use_fast_resize)
    {
        contents_container_->SetFastResize(false);
    }

    // Inform the InfoBarContainer that the distance to the location icon may have
    // changed.  We have to do this after the block above so that the toolbars are
    // laid out correctly for calculating the maximum arrow height below.
    {
        const LocationIconView* location_icon_view =
            toolbar_->location_bar()->location_icon_view();
        // The +1 in the next line creates a 1-px gap between icon and arrow tip.
        gfx::Point icon_bottom(0, location_icon_view->GetImageBounds().bottom() -
            LocationBarView::kIconInternalPadding + 1);
        ConvertPointToView(location_icon_view, this, &icon_bottom);
        gfx::Point infobar_top(0, infobar_container_->GetVerticalOverlap(NULL));
        ConvertPointToView(infobar_container_, this, &infobar_top);

        AutoReset<CallState> resetter(&call_state,
            is_animating ? REENTRANT_FORCE_FAST_RESIZE : REENTRANT);
        infobar_container_->SetMaxTopArrowHeight(infobar_top.y() - icon_bottom.y());
    }

    // When transitioning from animating to not animating we need to make sure the
    // contents_container_ gets layed out. If we don't do this and the bounds
    // haven't changed contents_container_ won't get a Layout out and we'll end up
    // with a gray rect because the clip wasn't updated.  Note that a reentrant
    // call never needs to do this, because after it returns, the normal call
    // wrapping it will do it.
    if((call_state == NORMAL) && !is_animating)
    {
        contents_container_->InvalidateLayout();
        //contents_split_->Layout();
    }
}

void BrowserView::UpdateTitleBar()
{
    frame_->UpdateWindowTitle();
    if(ShouldShowWindowIcon() && !loading_animation_timer_.IsRunning())
    {
        frame_->UpdateWindowIcon();
    }
}

void BrowserView::BookmarkBarStateChanged(
    BookmarkBar::AnimateChangeType change_type)
{
    //if(bookmark_bar_view_.get())
    //{
    //    bookmark_bar_view_->SetBookmarkBarState(
    //        browser_->bookmark_bar_state(), change_type);
    //}
    if(MaybeShowBookmarkBar(browser_->GetSelectedTabContentsWrapper()))
    {
        Layout();
    }
}

void BrowserView::UpdateLoadingAnimations(bool should_animate)
{
    if(should_animate)
    {
        if(!loading_animation_timer_.IsRunning())
        {
            // Loads are happening, and the timer isn't running, so start it.
            last_animation_time_ = base::TimeTicks::Now();
            loading_animation_timer_.Start(
                TimeDelta::FromMilliseconds(kLoadingAnimationFrameTimeMs), this,
                &BrowserView::LoadingAnimationCallback);
        }
    }
    else
    {
        if(loading_animation_timer_.IsRunning())
        {
            last_animation_time_ = base::TimeTicks();
            loading_animation_timer_.Stop();
            // Loads are now complete, update the state if a task was scheduled.
            LoadingAnimationCallback();
        }
    }
}

void BrowserView::SetStarredState(bool is_starred)
{
    GetLocationBarView()->SetStarToggled(is_starred);
}

gfx::Rect BrowserView::GetRestoredBounds() const
{
    return frame_->GetRestoredBounds();
}

gfx::Rect BrowserView::GetBounds() const
{
    return frame_->GetWindowScreenBounds();
}

bool BrowserView::IsMaximized() const
{
    return frame_->IsMaximized();
}

bool BrowserView::IsMinimized() const
{
    return frame_->IsMinimized();
}

void BrowserView::SetFullscreen(bool fullscreen)
{
    if(IsFullscreen() == fullscreen)
    {
        return; // Nothing to do.
    }

    ProcessFullscreen(fullscreen);
}

bool BrowserView::IsFullscreen() const
{
    return frame_->IsFullscreen();
}

bool BrowserView::IsFullscreenBubbleVisible() const
{
    return false;
    //return fullscreen_bubble_.get() ? true : false;
}

void BrowserView::FullScreenStateChanged()
{
    ProcessFullscreen(IsFullscreen());
}

void BrowserView::RestoreFocus()
{
    TabContents* selected_tab_contents = GetSelectedTabContents();
    if(selected_tab_contents)
    {
        selected_tab_contents->view()->RestoreFocus();
    }
}

LocationBar* BrowserView::GetLocationBar() const
{
    return GetLocationBarView();
}

void BrowserView::SetFocusToLocationBar(bool select_all)
{
    LocationBarView* location_bar = GetLocationBarView();
    if(location_bar->IsFocusableInRootView())
    {
        // Location bar got focus.
        location_bar->FocusLocation(select_all);
    }
    else
    {
        // If none of location bar/compact navigation bar got focus,
        // then clear focus.
        view::FocusManager* focus_manager = GetFocusManager();
        DCHECK(focus_manager);
        focus_manager->ClearFocus();
    }
}

void BrowserView::UpdateReloadStopState(bool is_loading, bool force)
{
    //ReloadButton* reload_button = NULL;
    //reload_button = toolbar_->reload_button();
    //reload_button->ChangeMode(
    //    is_loading ? ReloadButton::MODE_STOP : ReloadButton::MODE_RELOAD, force);
}

void BrowserView::UpdateToolbar(TabContentsWrapper* contents,
                                bool should_restore_state)
{
    toolbar_->Update(contents->tab_contents(), should_restore_state);
}

void BrowserView::FocusToolbar()
{
    // Start the traversal within the main toolbar, passing it the storage id
    // of the view where focus should be returned if the user exits the toolbar.
    SaveFocusedView();
    toolbar_->SetPaneFocus(last_focused_view_storage_id_, NULL);
}

void BrowserView::FocusBookmarksToolbar()
{
    //if(active_bookmark_bar_ && bookmark_bar_view_->IsVisible())
    //{
    //    SaveFocusedView();
    //    bookmark_bar_view_->SetPaneFocus(last_focused_view_storage_id_, NULL);
    //}
}

void BrowserView::FocusAppMenu()
{
    // Chrome doesn't have a traditional menu bar, but it has a menu button in the
    // main toolbar that plays the same role.  If the user presses a key that
    // would typically focus the menu bar, tell the toolbar to focus the menu
    // button.  If the user presses the key again, return focus to the previous
    // location.
    //
    // Not used on the Mac, which has a normal menu bar.
    if(toolbar_->IsAppMenuFocused())
    {
        RestoreFocus();
    }
    else
    {
        SaveFocusedView();
        // TODO(mad): find out how to add this to compact nav view.
        toolbar_->SetPaneFocusAndFocusAppMenu(last_focused_view_storage_id_);
    }
}

void BrowserView::RotatePaneFocus(bool forwards)
{
    // This gets called when the user presses F6 (forwards) or Shift+F6
    // (backwards) to rotate to the next pane. Here, our "panes" are the
    // tab contents and each of our accessible toolbars, infobars, downloads
    // shelf, etc.  When a pane has focus, all of its controls are accessible
    // in the tab traversal, and the tab traversal is "trapped" within that pane.
    //
    // Get a vector of all panes in the order we want them to be focused,
    // with NULL to represent the tab contents getting focus. If one of these
    // is currently invisible or has no focusable children it will be
    // automatically skipped.
    std::vector<AccessiblePaneView*> accessible_panes;
    GetAccessiblePanes(&accessible_panes);
    int pane_count = static_cast<int>(accessible_panes.size());

    std::vector<view::View*> accessible_views(
        accessible_panes.begin(), accessible_panes.end());
    //accessible_views.push_back(GetTabContentsContainerView());
    //if(sidebar_container_ && sidebar_container_->IsVisible())
    //{
    //    accessible_views.push_back(GetSidebarContainerView());
    //}
    int count = static_cast<int>(accessible_views.size());

    // Figure out which view (if any) currently has the focus.
    view::View* focused_view = GetFocusManager()->GetFocusedView();
    int index = -1;
    if(focused_view)
    {
        for(int i=0; i<count; ++i)
        {
            if(accessible_views[i]==focused_view ||
                accessible_views[i]->Contains(focused_view))
            {
                index = i;
                break;
            }
        }
    }

    // If the focus isn't currently in a pane, save the focus so we
    // can restore it if the user presses Escape.
    if(focused_view && index >= pane_count)
    {
        SaveFocusedView();
    }

    // Try to focus the next pane; if SetPaneFocusAndFocusDefault returns
    // false it means the pane didn't have any focusable controls, so skip
    // it and try the next one.
    for(;;)
    {
        if(forwards)
        {
            index = (index + 1) % count;
        }
        else
        {
            index = ((index - 1) + count) % count;
        }

        if(index < pane_count)
        {
            if(accessible_panes[index]->SetPaneFocusAndFocusDefault(
                last_focused_view_storage_id_))
            {
                break;
            }
        }
        else
        {
            accessible_views[index]->RequestFocus();
            break;
        }
    }
}

void BrowserView::SaveFocusedView()
{
    view::ViewStorage* view_storage = view::ViewStorage::GetInstance();
    if(view_storage->RetrieveView(last_focused_view_storage_id_))
    {
        view_storage->RemoveView(last_focused_view_storage_id_);
    }
    view::View* focused_view = GetFocusManager()->GetFocusedView();
    if(focused_view)
    {
        view_storage->StoreView(last_focused_view_storage_id_, focused_view);
    }
}

void BrowserView::DestroyBrowser()
{
    // After this returns other parts of Chrome are going to be shutdown. Close
    // the window now so that we are deleted immediately and aren't left holding
    // references to deleted objects.
    GetWidget()->RemoveObserver(this);
    frame_->CloseNow();
}

bool BrowserView::IsBookmarkBarVisible() const
{
    return browser_->SupportsWindowFeature(Browser::FEATURE_BOOKMARKBAR) &&
        active_bookmark_bar_ &&
        (active_bookmark_bar_->GetPreferredSize().height() != 0);
}

bool BrowserView::IsBookmarkBarAnimating() const
{
    return bookmark_bar_view_.get() && bookmark_bar_view_->is_animating();
}

bool BrowserView::IsTabStripEditable() const
{
    return tabstrip_->IsTabStripEditable();
}

bool BrowserView::IsToolbarVisible() const
{
    return browser_->SupportsWindowFeature(Browser::FEATURE_TOOLBAR) ||
        browser_->SupportsWindowFeature(Browser::FEATURE_LOCATIONBAR);
}

void BrowserView::DisableInactiveFrame()
{
    frame_->DisableInactiveRendering();
}

void BrowserView::ToggleBookmarkBar()
{
    //bookmark_utils::ToggleWhenVisible(browser_->profile());
}

void BrowserView::ShowAboutChromeDialog()
{
    DoShowAboutChromeDialog();
}

view::Widget* BrowserView::DoShowAboutChromeDialog()
{
    //return browser::ShowAboutChromeView(GetWidget()->GetNativeWindow(),
    //    browser_->profile());
    return NULL;
}

void BrowserView::ShowUpdateChromeDialog()
{
    //UpdateRecommendedMessageBox::ShowMessageBox(GetWidget()->GetNativeWindow());
}

void BrowserView::ShowTaskManager()
{
    //browser::ShowTaskManager();
}

void BrowserView::ShowBackgroundPages()
{
    //browser::ShowBackgroundPages();
}

void BrowserView::ShowBookmarkBubble(const Url& url, bool already_bookmarked)
{
    GetLocationBarView()->ShowStarBubble(url, !already_bookmarked);
}

void BrowserView::ShowRepostFormWarningDialog(TabContents* tab_contents)
{
    //browser::ShowRepostFormWarningDialog(GetNativeHandle(), tab_contents);
}

void BrowserView::ShowCollectedCookiesDialog(TabContents* tab_contents)
{
    //browser::ShowCollectedCookiesDialog(tab_contents);
}

void BrowserView::UserChangedTheme()
{
    frame_->FrameTypeChanged();
}

int BrowserView::GetExtraRenderViewHeight() const
{
    // Currently this is only used on linux.
    return 0;
}

void BrowserView::TabContentsFocused(TabContents* tab_contents)
{
    contents_container_->TabContentsFocused(tab_contents);
}

void BrowserView::ShowAppMenu()
{
    // TODO(mad): find out how to add this to compact nav view.
    toolbar_->app_menu()->Activate();
}

// TODO(devint): http://b/issue?id=1117225 Cut, Copy, and Paste are always
// enabled in the page menu regardless of whether the command will do
// anything. When someone selects the menu item, we just act as if they hit
// the keyboard shortcut for the command by sending the associated key press
// to windows. The real fix to this bug is to disable the commands when they
// won't do anything. We'll need something like an overall clipboard command
// manager to do that.
void BrowserView::Cut()
{
    //ui_controls::SendKeyPress(GetNativeHandle(), ui::VKEY_X,
    //    true, false, false, false);
}

void BrowserView::Copy()
{
    //ui_controls::SendKeyPress(GetNativeHandle(), ui::VKEY_C,
    //    true, false, false, false);
}

void BrowserView::Paste()
{
    //ui_controls::SendKeyPress(GetNativeHandle(), ui::VKEY_V,
    //    true, false, false, false);
}

void BrowserView::ToggleTabStripMode()
{
    InitTabStrip(browser_->tabstrip_model());
    frame_->TabStripDisplayModeChanged();
}

WindowOpenDisposition BrowserView::GetDispositionForPopupBounds(
    const gfx::Rect& bounds)
{
    return NEW_POPUP;
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, BrowserWindowTesting implementation:

LocationBarView* BrowserView::GetLocationBarView() const
{
    return toolbar_ ? toolbar_->location_bar() : NULL;
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, TabStripModelObserver implementation:

void BrowserView::TabDetachedAt(TabContentsWrapper* contents, int index)
{
    // We use index here rather than comparing |contents| because by this time
    // the model has already removed |contents| from its list, so
    // browser_->GetSelectedTabContents() will return NULL or something else.
    if(index == browser_->tabstrip_model()->active_index())
    {
        // We need to reset the current tab contents to NULL before it gets
        // freed. This is because the focus manager performs some operations
        // on the selected TabContents when it is removed.
        contents_container_->ChangeTabContents(NULL);
        infobar_container_->ChangeTabContents(NULL);
        UpdateSidebarForContents(NULL);
    }
}

void BrowserView::TabDeactivated(TabContentsWrapper* contents)
{
    // We do not store the focus when closing the tab to work-around bug 4633.
    // Some reports seem to show that the focus manager and/or focused view can
    // be garbage at that point, it is not clear why.
    if(!contents->tab_contents()->is_being_destroyed())
    {
        contents->view()->StoreFocus();
    }
}

void BrowserView::ActiveTabChanged(TabContentsWrapper* old_contents,
                                   TabContentsWrapper* new_contents,
                                   int index,
                                   bool user_gesture)
{
    ProcessTabSelected(new_contents, true);
}

void BrowserView::TabReplacedAt(TabStripModel* tab_strip_model,
                                TabContentsWrapper* old_contents,
                                TabContentsWrapper* new_contents,
                                int index)
{
    if(index != browser_->tabstrip_model()->active_index())
    {
        return;
    }

    if(contents_->preview_tab_contents() == new_contents->tab_contents())
    {
        // If 'preview' is becoming active, swap the 'active' and 'preview' and
        // delete what was the active.
        contents_->MakePreviewContentsActiveContents();
        TabContentsContainer* old_container = contents_container_;
        contents_container_ = preview_container_;
        old_container->ChangeTabContents(NULL);
        delete old_container;
        preview_container_ = NULL;

        // Update the UI for what was the preview contents and is now active. Pass
        // in false to ProcessTabSelected as new_contents is already parented
        // correctly.
        ProcessTabSelected(new_contents, false);
    }
    else
    {
        // Update the UI for the new contents. Pass in true to ProcessTabSelected as
        // new_contents is not parented correctly.
        ProcessTabSelected(new_contents, true);
    }
}

void BrowserView::TabStripEmpty()
{
    // Make sure all optional UI is removed before we are destroyed, otherwise
    // there will be consequences (since our view hierarchy will still have
    // references to freed views).
    UpdateUIForContents(NULL);
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, ui::SimpleMenuModel::Delegate implementation:

bool BrowserView::IsCommandIdChecked(int command_id) const
{
    // TODO(beng): encoding menu.
    // No items in our system menu are check-able.
    return false;
}

bool BrowserView::IsCommandIdEnabled(int command_id) const
{
    return false;//browser_->command_updater()->IsCommandEnabled(command_id);
}

bool BrowserView::GetAcceleratorForCommandId(int command_id,
                                             ui::Accelerator* accelerator)
{
    // Let's let the ToolbarView own the canonical implementation of this method.
    return toolbar_->GetAcceleratorForCommandId(command_id, accelerator);
}

bool BrowserView::IsItemForCommandIdDynamic(int command_id) const
{
    return command_id == IDC_RESTORE_TAB;
}

string16 BrowserView::GetLabelForCommandId(int command_id) const
{
    DCHECK(command_id == IDC_RESTORE_TAB);

    int string_id = IDS_RESTORE_TAB;
    //if(IsCommandIdEnabled(command_id))
    //{
    //    TabRestoreService* trs =
    //        TabRestoreServiceFactory::GetForProfile(browser_->profile());
    //    if(trs && trs->entries().front()->type == TabRestoreService::WINDOW)
    //    {
    //        string_id = IDS_RESTORE_WINDOW;
    //    }
    //}
    return ui::GetStringUTF16(string_id);
}

void BrowserView::ExecuteCommand(int command_id)
{
    browser_->ExecuteCommandIfEnabled(command_id);
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, view::WindowDelegate implementation:

bool BrowserView::CanResize() const
{
    return true;
}

bool BrowserView::CanMaximize() const
{
    return true;
}

bool BrowserView::CanActivate() const
{
    return !ActivateAppModalDialog();
}

bool BrowserView::IsModal() const
{
    return false;
}

std::wstring BrowserView::GetWindowTitle() const
{
    return browser_->GetWindowTitleForCurrentTab();
}

std::wstring BrowserView::GetAccessibleWindowTitle() const
{
    return GetWindowTitle();
}

view::View* BrowserView::GetInitiallyFocusedView()
{
    // We set the frame not focus on creation so this should never be called.
    NOTREACHED();
    return NULL;
}

bool BrowserView::ShouldShowWindowTitle() const
{
    return browser_->SupportsWindowFeature(Browser::FEATURE_TITLEBAR);
}

SkBitmap BrowserView::GetWindowAppIcon()
{
    return GetWindowIcon();
}

SkBitmap BrowserView::GetWindowIcon()
{
    return SkBitmap();
}

bool BrowserView::ShouldShowWindowIcon() const
{
    return browser_->SupportsWindowFeature(Browser::FEATURE_TITLEBAR);
}

bool BrowserView::ExecuteWindowsCommand(int command_id)
{
    // This function handles WM_SYSCOMMAND, WM_APPCOMMAND, and WM_COMMAND.
    if(command_id == IDC_DEBUG_FRAME_TOGGLE)
    {
        GetWidget()->DebugToggleFrameType();
    }
    // Translate WM_APPCOMMAND command ids into a command id that the browser
    // knows how to handle.
    int command_id_from_app_command = GetCommandIDForAppCommandID(command_id);
    if(command_id_from_app_command != -1)
    {
        command_id = command_id_from_app_command;
    }

    return browser_->ExecuteCommandIfEnabled(command_id);
}

std::wstring BrowserView::GetWindowName() const
{
    return UTF8ToWide(browser_->GetWindowPlacementKey());
}

void BrowserView::SaveWindowPlacement(const gfx::Rect& bounds,
                                      ui::WindowShowState show_state)
{
    // TODO(dhollowa): Add support for session restore of minimized state.
    // http://crbug.com/43274

    // If IsFullscreen() is true, we've just changed into fullscreen mode, and
    // we're catching the going-into-fullscreen sizing and positioning calls,
    // which we want to ignore.
    if(!IsFullscreen() && browser_->ShouldSaveWindowPlacement())
    {
        WidgetDelegate::SaveWindowPlacement(bounds, show_state);
        browser_->SaveWindowPlacement(bounds, show_state);
    }
}

bool BrowserView::GetSavedWindowPlacement(gfx::Rect* bounds,
                                          ui::WindowShowState* show_state) const
{
    *bounds = browser_->GetSavedWindowBounds();
    *show_state = browser_->GetSavedWindowShowState();

    if(browser_->is_type_popup() || browser_->is_type_panel())
    {
        // We are a popup window. The value passed in |bounds| represents two
        // pieces of information:
        // - the position of the window, in screen coordinates (outer position).
        // - the size of the content area (inner size).
        // We need to use these values to determine the appropriate size and
        // position of the resulting window.
        if(IsToolbarVisible())
        {
            // If we're showing the toolbar, we need to adjust |*bounds| to include
            // its desired height, since the toolbar is considered part of the
            // window's client area as far as GetWindowBoundsForClientBounds is
            // concerned...
            bounds->set_height(
                bounds->height() + toolbar_->GetPreferredSize().height());
        }

        gfx::Rect window_rect = frame_->non_client_view()->
            GetWindowBoundsForClientBounds(*bounds);
        window_rect.set_origin(bounds->origin());

        // When we are given x/y coordinates of 0 on a created popup window,
        // assume none were given by the window.open() command.
        if(window_rect.x() == 0 && window_rect.y() == 0)
        {
            //gfx::Size size = window_rect.size();
            //window_rect.set_origin(WindowSizer::GetDefaultPopupOrigin(size));
        }

        *bounds = window_rect;
    }

    // We return true because we can _always_ locate reasonable bounds using the
    // WindowSizer, and we don't want to trigger the Window's built-in "size to
    // default" handling because the browser window has no default preferred
    // size.
    return true;
}

view::View* BrowserView::GetContentsView()
{
    return contents_container_;
}

view::ClientView* BrowserView::CreateClientView(view::Widget* widget)
{
    return this;
}

void BrowserView::OnWidgetActivationChanged(view::Widget* widget, bool active)
{
    if(active)
    {
        BrowserList::SetLastActive(browser_.get());
        browser_->OnWindowActivated();
    }
}

void BrowserView::OnWindowBeginUserBoundsChange()
{
    TabContents* tab_contents = GetSelectedTabContents();
    if(!tab_contents)
    {
        return;
    }
    //RenderViewHost* rvh = tab_contents->render_view_host();
    //rvh->Send(new ViewMsg_MoveOrResizeStarted(rvh->routing_id()));
}

void BrowserView::OnWidgetMove()
{
    if(!initialized_)
    {
        // Creating the widget can trigger a move. Ignore it until we've initialized
        // things.
        return;
    }

    // Cancel any tabstrip animations, some of them may be invalidated by the
    // window being repositioned.
    // Comment out for one cycle to see if this fixes dist tests.
    // tabstrip_->DestroyDragController();

    BrowserBubbleHost::WindowMoved();

    //browser::HideBookmarkBubbleView();

    // Close the omnibox popup, if any.
    //LocationBarView* location_bar_view = GetLocationBarView();
    //if(location_bar_view)
    //{
    //    location_bar_view->location_entry()->ClosePopup();
    //}
}

view::Widget* BrowserView::GetWidget()
{
    return View::GetWidget();
}

const view::Widget* BrowserView::GetWidget() const
{
    return View::GetWidget();
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, view::ClientView overrides:

bool BrowserView::CanClose()
{
    // You cannot close a frame for which there is an active originating drag
    // session.
    if(tabstrip_ && !tabstrip_->IsTabStripCloseable())
    {
        return false;
    }

    // Give beforeunload handlers the chance to cancel the close before we hide
    // the window below.
    if(!browser_->ShouldCloseWindow())
    {
        return false;
    }

    if(!browser_->tabstrip_model()->empty())
    {
        // Tab strip isn't empty.  Hide the frame (so it appears to have closed
        // immediately) and close all the tabs, allowing the renderers to shut
        // down. When the tab strip is empty we'll be called back again.
        frame_->Hide();
        browser_->OnWindowClosing();
        return false;
    }

    return true;
}

int BrowserView::NonClientHitTest(const gfx::Point& point)
{
    // The following code is not in the LayoutManager because it's
    // independent of layout and also depends on the ResizeCorner which
    // is private.
    if(!frame_->IsMaximized() && !frame_->IsFullscreen())
    {
        RECT client_rect;
        ::GetClientRect(frame_->GetNativeWindow(), &client_rect);
        gfx::Size resize_corner_size = ResizeCorner::GetSize();
        gfx::Rect resize_corner_rect(client_rect.right - resize_corner_size.width(),
            client_rect.bottom - resize_corner_size.height(),
            resize_corner_size.width(), resize_corner_size.height());
        bool rtl_dir = base::i18n::IsRTL();
        if(rtl_dir)
        {
            resize_corner_rect.set_x(0);
        }
        if(resize_corner_rect.Contains(point))
        {
            if(rtl_dir)
            {
                return HTBOTTOMLEFT;
            }
            return HTBOTTOMRIGHT;
        }
    }

    return GetBrowserViewLayout()->NonClientHitTest(point);
}

gfx::Size BrowserView::GetMinimumSize()
{
    return GetBrowserViewLayout()->GetMinimumSize();
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, protected

void BrowserView::GetAccessiblePanes(std::vector<AccessiblePaneView*>* panes)
{
    // This should be in the order of pane traversal of the panes using F6.
    // If one of these is invisible or has no focusable children, it will be
    // automatically skipped.
    panes->push_back(toolbar_);
    if(bookmark_bar_view_.get())
    {
        panes->push_back(bookmark_bar_view_.get());
    }
    if(infobar_container_)
    {
        panes->push_back(infobar_container_);
    }
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, view::View overrides:

std::string BrowserView::GetClassName() const
{
    return kViewClassName;
}

void BrowserView::Layout()
{
    if(ignore_layout_)
    {
        return;
    }
    view::View::Layout();
}

void BrowserView::PaintChildren(gfx::Canvas* canvas)
{
    // Paint the |infobar_container_| last so that it may paint its
    // overlapping tabs.
    for(int i=0; i<child_count(); ++i)
    {
        View* child = child_at(i);
        if(child != infobar_container_)
        {
            child->Paint(canvas);
        }
    }

    infobar_container_->Paint(canvas);
}

void BrowserView::ViewHierarchyChanged(bool is_add,
                                       view::View* parent,
                                       view::View* child)
{
    if(is_add && child==this && GetWidget() && !initialized_)
    {
        Init();
        initialized_ = true;
    }
}

void BrowserView::ChildPreferredSizeChanged(View* child)
{
    Layout();
}

void BrowserView::GetAccessibleState(ui::AccessibleViewState* state)
{
    state->name = ui::GetStringUTF16(IDS_PRODUCT_NAME);
    state->role = ui::AccessibilityTypes::ROLE_CLIENT;
}

SkColor BrowserView::GetInfoBarSeparatorColor() const
{
    // NOTE: Keep this in sync with ToolbarView::OnPaint()!
    return (IsTabStripVisible() || !frame_->ShouldUseNativeFrame()) ?
        ui::ResourceBundle::toolbar_separator_color : SK_ColorBLACK;
}

void BrowserView::InfoBarContainerStateChanged(bool is_animating)
{
    ToolbarSizeChanged(is_animating);
}

bool BrowserView::DrawInfoBarArrows(int* x) const
{
    if(x)
    {
        const LocationIconView* location_icon_view =
            toolbar_->location_bar()->location_icon_view();
        gfx::Point icon_center(location_icon_view->GetImageBounds().CenterPoint());
        ConvertPointToView(location_icon_view, this, &icon_center);
        *x = icon_center.x();
    }
    return true;
}

bool BrowserView::SplitHandleMoved(view::SingleSplitView* view)
{
    for(int i=0; i<view->child_count(); ++i)
    {
        view->child_at(i)->InvalidateLayout();
    }
    SchedulePaint();
    Layout();
    return false;
}

view::LayoutManager* BrowserView::CreateLayoutManager() const
{
    return new BrowserViewLayout;
}

void BrowserView::InitTabStrip(TabStripModel* model)
{
    // Throw away the existing tabstrip if we're switching display modes.
    scoped_ptr<AbstractTabStripView> old_strip(tabstrip_);
    if(tabstrip_)
    {
        tabstrip_->parent()->RemoveChildView(tabstrip_);
    }

    tabstrip_ = CreateTabStrip(browser_.get(), this, model);
}

ToolbarView* BrowserView::CreateToolbar() const
{
    return new ToolbarView(browser_.get());
}

void BrowserView::Init()
{
    GetWidget()->AddObserver(this);

    SetLayoutManager(CreateLayoutManager());
    // Stow a pointer to this object onto the window handle so that we can get at
    // it later when all we have is a native view.
    GetWidget()->SetNativeWindowProperty(kBrowserViewKey, this);

    // Stow a pointer to the browser's profile onto the window handle so that we
    // can get it later when all we have is a native view.
    //GetWidget()->SetNativeWindowProperty(Profile::kProfileKey,
    //    browser_->profile());

    LoadAccelerators();

    InitTabStrip(browser_->tabstrip_model());

    SetToolbar(CreateToolbar());

    infobar_container_ = new InfoBarContainerView(this);
    AddChildView(infobar_container_);

    contents_container_ = new TabContentsContainer;
    contents_ = new ContentsContainer(contents_container_);

    SkColor bg_color = SkColorSetRGB(230, 230, 230);

    //bool sidebar_allowed = SidebarManager::IsSidebarAllowed();
    //if(sidebar_allowed)
    //{
    //    sidebar_container_ = new TabContentsContainer;
    //    sidebar_container_->set_id(VIEW_ID_SIDE_BAR_CONTAINER);
    //    sidebar_container_->SetVisible(false);

    //    sidebar_split_ = new view::SingleSplitView(
    //        contents_,
    //        sidebar_container_,
    //        view::SingleSplitView::HORIZONTAL_SPLIT,
    //        this);
    //    sidebar_split_->set_id(VIEW_ID_SIDE_BAR_SPLIT);
    //    sidebar_split_->SetAccessibleName(
    //        ui::GetStringUTF16(IDS_ACCNAME_SIDE_BAR));
    //    sidebar_split_->set_background(
    //        view::Background::CreateSolidBackground(bg_color));
    //}

    view::View* contents_view = contents_;
    //if(sidebar_allowed)
    //{
    //    contents_view = sidebar_split_;
    //}

    AddChildView(contents_view);
    set_contents_view(contents_view);

    //contents_split_ = new view::SingleSplitView(
    //    contents_view,
    //    devtools_container_,
    //    view::SingleSplitView::VERTICAL_SPLIT,
    //    this);
    //contents_split_->set_id(VIEW_ID_CONTENTS_SPLIT);
    //contents_split_->SetAccessibleName(
    //    ui::GetStringUTF16(IDS_ACCNAME_WEB_CONTENTS));
    //contents_split_->set_background(
    //    view::Background::CreateSolidBackground(bg_color));
    //AddChildView(contents_split_);
    //set_contents_view(contents_split_);

    InitSystemMenu();

    // We're now initialized and ready to process Layout requests.
    ignore_layout_ = false;
}

void BrowserView::LoadingAnimationCallback()
{
    base::TimeTicks now = base::TimeTicks::Now();
    if(!last_animation_time_.is_null())
    {
        UMA_HISTOGRAM_TIMES("Tabs.LoadingAnimationTime",
            now - last_animation_time_);
    }
    last_animation_time_ = now;
    if(browser_->is_type_tabbed())
    {
        // Loading animations are shown in the tab for tabbed windows.  We check the
        // browser type instead of calling IsTabStripVisible() because the latter
        // will return false for fullscreen windows, but we still need to update
        // their animations (so that when they come out of fullscreen mode they'll
        // be correct).
        tabstrip_->UpdateLoadingAnimations();
    }
    else if(ShouldShowWindowIcon())
    {
        // ... or in the window icon area for popups and app windows.
        TabContents* tab_contents = browser_->GetSelectedTabContents();
        // GetSelectedTabContents can return NULL for example under Purify when
        // the animations are running slowly and this function is called on a timer
        // through LoadingAnimationCallback.
        frame_->UpdateThrobber(tab_contents && tab_contents->IsLoading());
    }
}

// BrowserView, private --------------------------------------------------------

void BrowserView::InitSystemMenu()
{
    system_menu_contents_.reset(new view::SystemMenuModel(this));
    // We add the menu items in reverse order so that insertion_index never needs
    // to change.
    if(IsBrowserTypeNormal())
    {
        BuildSystemMenuForBrowserWindow();
    }
    else
    {
        BuildSystemMenuForAppOrPopupWindow();
    }
    system_menu_.reset(
        new view::NativeMenuWin(system_menu_contents_.get(),
        frame_->GetNativeWindow()));
    system_menu_->Rebuild();
}

BrowserViewLayout* BrowserView::GetBrowserViewLayout() const
{
    return static_cast<BrowserViewLayout*>(GetLayoutManager());
}

bool BrowserView::MaybeShowBookmarkBar(TabContentsWrapper* contents)
{
    view::View* new_bookmark_bar_view = NULL;
    if(browser_->SupportsWindowFeature(Browser::FEATURE_BOOKMARKBAR) &&
        contents)
    {
        if(!bookmark_bar_view_.get())
        {
            bookmark_bar_view_.reset(new BookmarkBarView(browser_.get()));
            bookmark_bar_view_->set_parent_owned(false);
            bookmark_bar_view_->set_background(
                new BookmarkExtensionBackground(this, bookmark_bar_view_.get(),
                browser_.get()));
            bookmark_bar_view_->SetBookmarkBarState(
                browser_->bookmark_bar_state(),
                BookmarkBar::DONT_ANIMATE_STATE_CHANGE);
        }
        //bookmark_bar_view_->SetPageNavigator(contents->tab_contents());
        new_bookmark_bar_view = bookmark_bar_view_.get();
    }
    return UpdateChildViewAndLayout(new_bookmark_bar_view, &active_bookmark_bar_);
}

bool BrowserView::MaybeShowInfoBar(TabContentsWrapper* contents)
{
    // TODO(beng): Remove this function once the interface between
    //             InfoBarContainer, DownloadShelfView and TabContents and this
    //             view is sorted out.
    return true;
}

void BrowserView::UpdateSidebar()
{
    UpdateSidebarForContents(GetSelectedTabContentsWrapper());
    Layout();
}

void BrowserView::UpdateSidebarForContents(TabContentsWrapper* tab_contents)
{
    if(!sidebar_container_)
    {
        return; // Happens when sidebar is not allowed.
    }
    //if(!SidebarManager::GetInstance())
    //{
    //    return; // Happens only in tests.
    //}

    //TabContents* sidebar_contents = NULL;
    //if(tab_contents)
    //{
    //    SidebarContainer* client_host = SidebarManager::GetInstance()->
    //        GetActiveSidebarContainerFor(tab_contents->tab_contents());
    //    if(client_host)
    //    {
    //        sidebar_contents = client_host->sidebar_contents();
    //    }
    //}

    //bool visible = NULL != sidebar_contents &&
    //    browser_->SupportsWindowFeature(Browser::FEATURE_SIDEBAR);

    //bool should_show = visible && !sidebar_container_->IsVisible();
    //bool should_hide = !visible && sidebar_container_->IsVisible();

    //// Update sidebar content.
    //TabContents* old_contents = sidebar_container_->tab_contents();
    //sidebar_container_->ChangeTabContents(sidebar_contents);
    ////SidebarManager::GetInstance()->
    ////    NotifyStateChanges(old_contents, sidebar_contents);

    //// Update sidebar UI width.
    //if(should_show)
    //{
    //    // Restore split offset.
    //    int sidebar_width = g_browser_process->local_state()->GetInteger(
    //        prefs::kExtensionSidebarWidth);
    //    if(sidebar_width < 0)
    //    {
    //        // Initial load, set to default value.
    //        sidebar_width = sidebar_split_->width() / 7;
    //    }
    //    // Make sure user can see both panes.
    //    int min_sidebar_width = sidebar_split_->GetMinimumSize().width();
    //    sidebar_width = std::min(sidebar_split_->width() - min_sidebar_width,
    //        std::max(min_sidebar_width, sidebar_width));

    //    sidebar_split_->set_divider_offset(
    //        sidebar_split_->width() - sidebar_width);

    //    sidebar_container_->SetVisible(true);
    //    sidebar_split_->InvalidateLayout();
    //    Layout();
    //}
    //else if(should_hide)
    //{
    //    sidebar_container_->SetVisible(false);
    //    sidebar_split_->InvalidateLayout();
    //    Layout();
    //}
}

void BrowserView::UpdateUIForContents(TabContentsWrapper* contents)
{
    bool needs_layout = MaybeShowBookmarkBar(contents);
    needs_layout |= MaybeShowInfoBar(contents);
    if(needs_layout)
    {
        Layout();
    }
}

bool BrowserView::UpdateChildViewAndLayout(view::View* new_view,
                                           view::View** old_view)
{
    DCHECK(old_view);
    if(*old_view == new_view)
    {
        // The views haven't changed, if the views pref changed schedule a layout.
        if(new_view)
        {
            if(new_view->GetPreferredSize().height() != new_view->height())
            {
                return true;
            }
        }
        return false;
    }

    // The views differ, and one may be null (but not both). Remove the old
    // view (if it non-null), and add the new one (if it is non-null). If the
    // height has changed, schedule a layout, otherwise reuse the existing
    // bounds to avoid scheduling a layout.

    int current_height = 0;
    if(*old_view)
    {
        current_height = (*old_view)->height();
        RemoveChildView(*old_view);
    }

    int new_height = 0;
    if(new_view)
    {
        new_height = new_view->GetPreferredSize().height();
        AddChildView(new_view);
    }
    bool changed = false;
    if(new_height != current_height)
    {
        changed = true;
    }
    else if(new_view && *old_view)
    {
        // The view changed, but the new view wants the same size, give it the
        // bounds of the last view and have it repaint.
        new_view->SetBoundsRect((*old_view)->bounds());
        new_view->SchedulePaint();
    }
    else if(new_view)
    {
        DCHECK_EQ(0, new_height);
        // The heights are the same, but the old view is null. This only happens
        // when the height is zero. Zero out the bounds.
        new_view->SetBounds(0, 0, 0, 0);
    }
    *old_view = new_view;
    return changed;
}

void BrowserView::ProcessFullscreen(bool fullscreen)
{
    // Reduce jankiness during the following position changes by:
    //   * Hiding the window until it's in the final position
    //   * Ignoring all intervening Layout() calls, which resize the webpage and
    //     thus are slow and look ugly
    //ignore_layout_ = true;
    //LocationBarView* location_bar = GetLocationBarView();
    //OmniboxViewWin* omnibox_view =
    //    static_cast<OmniboxViewWin*>(location_bar->location_entry());
    //if(!fullscreen)
    //{
    //    // Hide the fullscreen bubble as soon as possible, since the mode toggle can
    //    // take enough time for the user to notice.
    //    fullscreen_bubble_.reset();
    //}
    //else
    //{
    //    // Move focus out of the location bar if necessary.
    //    view::FocusManager* focus_manager = GetFocusManager();
    //    DCHECK(focus_manager);
    //    if(focus_manager->GetFocusedView() == location_bar)
    //    {
    //        focus_manager->ClearFocus();
    //    }

    //    // If we don't hide the edit and force it to not show until we come out of
    //    // fullscreen, then if the user was on the New Tab Page, the edit contents
    //    // will appear atop the web contents once we go into fullscreen mode.  This
    //    // has something to do with how we move the main window while it's hidden;
    //    // if we don't hide the main window below, we don't get this problem.
    //    omnibox_view->set_force_hidden(true);
    //    ShowWindow(omnibox_view->m_hWnd, SW_HIDE);
    //}
    //static_cast<view::NativeWidgetWin*>(frame_->native_widget())->PushForceHidden();

    //// Toggle fullscreen mode.
    //frame_->SetFullscreen(fullscreen);

    //browser_->WindowFullscreenStateChanged();

    //if(fullscreen)
    //{
    //    //bool is_kiosk =
    //    //    CommandLine::ForCurrentProcess()->HasSwitch(switches::kKioskMode);
    //    //if(!is_kiosk)
    //    //{
    //    //    fullscreen_bubble_.reset(new FullscreenExitBubbleViews(GetWidget(),
    //    //        browser_.get()));
    //    //}
    //}
    //else
    //{
    //    // Show the edit again since we're no longer in fullscreen mode.
    //    omnibox_view->set_force_hidden(false);
    //    ShowWindow(omnibox_view->m_hWnd, SW_SHOW);
    //}

    //// Undo our anti-jankiness hacks and force the window to re-layout now that
    //// it's in its final position.
    //ignore_layout_ = false;
    //Layout();
    //static_cast<view::NativeWidgetWin*>(frame_->native_widget())->
    //    PopForceHidden();
}

void BrowserView::LoadAccelerators()
{
    //HACCEL accelerator_table = AtlLoadAccelerators(IDR_MAINFRAME);
    //DCHECK(accelerator_table);

    //// We have to copy the table to access its contents.
    //int count = CopyAcceleratorTable(accelerator_table, 0, 0);
    //if(count == 0)
    //{
    //    // Nothing to do in that case.
    //    return;
    //}

    //ACCEL* accelerators = static_cast<ACCEL*>(malloc(sizeof(ACCEL) * count));
    //CopyAcceleratorTable(accelerator_table, accelerators, count);

    //view::FocusManager* focus_manager = GetFocusManager();
    //DCHECK(focus_manager);

    //// Let's fill our own accelerator table.
    //for(int i=0; i<count; ++i)
    //{
    //    bool alt_down = (accelerators[i].fVirt & FALT) == FALT;
    //    bool ctrl_down = (accelerators[i].fVirt & FCONTROL) == FCONTROL;
    //    bool shift_down = (accelerators[i].fVirt & FSHIFT) == FSHIFT;
    //    view::Accelerator accelerator(
    //        static_cast<ui::KeyboardCode>(accelerators[i].key),
    //        shift_down, ctrl_down, alt_down);
    //    accelerator_table_[accelerator] = accelerators[i].cmd;

    //    // Also register with the focus manager.
    //    focus_manager->RegisterAccelerator(accelerator, this);
    //}

    //// We don't need the Windows accelerator table anymore.
    //free(accelerators);
}

void BrowserView::BuildSystemMenuForBrowserWindow()
{
    system_menu_contents_->AddSeparator();
    //system_menu_contents_->AddItemWithStringId(IDC_TASK_MANAGER,
    //    IDS_TASK_MANAGER);
    system_menu_contents_->AddSeparator();
    system_menu_contents_->AddItemWithStringId(IDC_RESTORE_TAB, IDS_RESTORE_TAB);
    system_menu_contents_->AddItemWithStringId(IDC_NEW_TAB, IDS_NEW_TAB);
    //AddFrameToggleItems();
    // If it's a regular browser window with tabs, we don't add any more items,
    // since it already has menus (Page, Chrome).
}

void BrowserView::BuildSystemMenuForAppOrPopupWindow()
{
    system_menu_contents_->AddSeparator();
    //encoding_menu_contents_.reset(new EncodingMenuModel(browser_.get()));
    //system_menu_contents_->AddSubMenuWithStringId(IDC_ENCODING_MENU,
    //    IDS_ENCODING_MENU,
    //    encoding_menu_contents_.get());
    //zoom_menu_contents_.reset(new ZoomMenuModel(this));
    //system_menu_contents_->AddSubMenuWithStringId(IDC_ZOOM_MENU, IDS_ZOOM_MENU,
    //    zoom_menu_contents_.get());
    //system_menu_contents_->AddItemWithStringId(IDC_PRINT, IDS_PRINT);
    //system_menu_contents_->AddItemWithStringId(IDC_FIND, IDS_FIND);
    //system_menu_contents_->AddSeparator();
    //system_menu_contents_->AddItemWithStringId(IDC_PASTE, IDS_PASTE);
    //system_menu_contents_->AddItemWithStringId(IDC_COPY, IDS_COPY);
    //system_menu_contents_->AddItemWithStringId(IDC_CUT, IDS_CUT);
    //system_menu_contents_->AddSeparator();
    //if(browser_->is_app())
    //{
    //    system_menu_contents_->AddItemWithStringId(IDC_NEW_TAB,
    //        IDS_APP_MENU_NEW_WEB_PAGE);
    //}
    //else
    //{
    //    system_menu_contents_->AddItemWithStringId(IDC_SHOW_AS_TAB,
    //        IDS_SHOW_AS_TAB);
    //}
    //system_menu_contents_->AddItemWithStringId(IDC_COPY_URL,
    //    IDS_APP_MENU_COPY_URL);
    //system_menu_contents_->AddSeparator();
    //system_menu_contents_->AddItemWithStringId(IDC_RELOAD, IDS_APP_MENU_RELOAD);
    //system_menu_contents_->AddItemWithStringId(IDC_FORWARD,
    //    IDS_CONTENT_CONTEXT_FORWARD);
    //system_menu_contents_->AddItemWithStringId(IDC_BACK,
    //    IDS_CONTENT_CONTEXT_BACK);
}

int BrowserView::GetCommandIDForAppCommandID(int app_command_id) const
{
    switch(app_command_id)
    {
        // NOTE: The order here matches the APPCOMMAND declaration order in the
        // Windows headers.
    case APPCOMMAND_BROWSER_BACKWARD: return IDC_BACK;
    case APPCOMMAND_BROWSER_FORWARD:  return IDC_FORWARD;
    case APPCOMMAND_BROWSER_REFRESH:  return IDC_RELOAD;
    case APPCOMMAND_BROWSER_HOME:     return IDC_HOME;
    case APPCOMMAND_BROWSER_STOP:     return IDC_STOP;
    case APPCOMMAND_BROWSER_SEARCH:   return IDC_FOCUS_SEARCH;
    case APPCOMMAND_HELP:             return IDC_HELP_PAGE;
    case APPCOMMAND_NEW:              return IDC_NEW_TAB;
    case APPCOMMAND_OPEN:             return IDC_OPEN_FILE;
    case APPCOMMAND_CLOSE:            return IDC_CLOSE_TAB;
    case APPCOMMAND_SAVE:             return IDC_SAVE_PAGE;
    case APPCOMMAND_PRINT:            return IDC_PRINT;
    case APPCOMMAND_COPY:             return IDC_COPY;
    case APPCOMMAND_CUT:              return IDC_CUT;
    case APPCOMMAND_PASTE:            return IDC_PASTE;

        // TODO(pkasting): http://b/1113069 Handle these.
    case APPCOMMAND_UNDO:
    case APPCOMMAND_REDO:
    case APPCOMMAND_SPELL_CHECK:
    default:                          return -1;
    }
}

void BrowserView::ProcessTabSelected(TabContentsWrapper* new_contents,
                                     bool change_tab_contents)
{
    // Update various elements that are interested in knowing the current
    // TabContents.

    // When we toggle the NTP floating bookmarks bar and/or the info bar,
    // we don't want any TabContents to be attached, so that we
    // avoid an unnecessary resize and re-layout of a TabContents.
    if(change_tab_contents)
    {
        contents_container_->ChangeTabContents(NULL);
    }
    infobar_container_->ChangeTabContents(new_contents);
    if(bookmark_bar_view_.get())
    {
        bookmark_bar_view_->SetBookmarkBarState(
            browser_->bookmark_bar_state(),
            BookmarkBar::DONT_ANIMATE_STATE_CHANGE);
    }
    UpdateUIForContents(new_contents);
    if(change_tab_contents)
    {
        contents_container_->ChangeTabContents(new_contents->tab_contents());
    }
    UpdateSidebarForContents(new_contents);

    // TODO(beng): This should be called automatically by ChangeTabContents, but I
    //             am striving for parity now rather than cleanliness. This is
    //             required to make features like Duplicate Tab, Undo Close Tab,
    //             etc not result in sad tab.
    new_contents->tab_contents()->DidBecomeSelected();
    if(BrowserList::GetLastActive()==browser_ &&
        !browser_->tabstrip_model()->closing_all() && GetWidget()->IsVisible())
    {
        // We only restore focus if our window is visible, to avoid invoking blur
        // handlers when we are eventually shown.
        new_contents->view()->RestoreFocus();
    }

    // Update all the UI bits.
    UpdateTitleBar();
    // No need to update Toolbar because it's already updated in
    // browser.cc.
}

gfx::Size BrowserView::GetResizeCornerSize() const
{
    return ResizeCorner::GetSize();
}

void BrowserView::SetToolbar(ToolbarView* toolbar)
{
    if(toolbar_)
    {
        RemoveChildView(toolbar_);
        delete toolbar_;
    }
    toolbar_ = toolbar;
    if(toolbar)
    {
        AddChildView(toolbar_);
        toolbar_->Init();
    }
}

// static
BrowserWindow* BrowserWindow::CreateBrowserWindow(Browser* browser)
{
    // Create the view and the frame. The frame will attach itself via the view
    // so we don't need to do anything with the pointer.
    BrowserView* view = new BrowserView(browser);
    (new BrowserFrame(view))->InitBrowserFrame();
    return view;
}