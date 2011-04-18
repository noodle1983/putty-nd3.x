
#include "browser_view.h"

#include "view/base/view_prop.h"
#include "view/focus/view_storage.h"
#include "view/l10n/l10n_util.h"
#include "view/widget/widget.h"
#include "view/window/window.h"

#include "../../../../wanui_res/resource.h"

#include "../../../common/notification_service.h"

// The name of a key to store on the window handle so that other code can
// locate this object using just the handle.
static const char* const kBrowserViewKey = "__BROWSER_VIEW__";

///////////////////////////////////////////////////////////////////////////////
// BrowserView, public:

BrowserView::BrowserView(Browser* browser)
: view::ClientView(NULL, NULL),
last_focused_view_storage_id_(view::ViewStorage::GetInstance()->CreateStorageID()),
frame_(NULL),
browser_(browser),
initialized_(false),
ignore_layout_(true)
{
}

BrowserView::~BrowserView()
{
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
            view::ViewProp::GetValue(window, kBrowserViewKey));
    }
    return NULL;
}

gfx::Rect BrowserView::GetClientAreaBounds() const
{
    return gfx::Rect();
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

    return false;
}

bool BrowserView::GetAccelerator(int cmd_id, view::MenuAccelerator* accelerator)
{
    // Else, we retrieve the accelerator information from the accelerator table.
    std::map<view::Accelerator, int>::iterator it =
        accelerator_table_.begin();
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

void BrowserView::PrepareToRunSystemMenu(HMENU menu)
{
    system_menu_->UpdateStates();
}

bool BrowserView::IsPositionInWindowCaption(const gfx::Point& point)
{
    return GetBrowserViewLayout()->IsPositionInWindowCaption(point);
}

void BrowserView::FullScreenStateChanged()
{
    ProcessFullscreen(IsFullscreen());
}

void BrowserView::RestoreFocus()
{
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, BrowserWindow implementation:

void BrowserView::Show()
{
    // If the window is already visible, just activate it.
    if(frame_->GetWindow()->IsVisible())
    {
        frame_->GetWindow()->Activate();
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

    frame_->GetWindow()->Show();
}

void BrowserView::ShowInactive()
{
    view::Window* window = frame_->GetWindow();
    if(!window->IsVisible())
    {
        window->ShowInactive();
    }
}

void BrowserView::SetBounds(const gfx::Rect& bounds)
{
    SetFullscreen(false);
    GetWidget()->SetBounds(bounds);
}

void BrowserView::Close()
{
    frame_->GetWindow()->CloseWindow();
}

void BrowserView::Activate()
{
    frame_->GetWindow()->Activate();
}

void BrowserView::Deactivate()
{
    frame_->GetWindow()->Deactivate();
}

bool BrowserView::IsActive() const
{
    return frame_->GetWindow()->IsActive();
}

void BrowserView::FlashFrame()
{
    FLASHWINFO fwi;
    fwi.cbSize = sizeof(fwi);
    fwi.hwnd = frame_->GetWindow()->GetNativeWindow();
    fwi.dwFlags = FLASHW_ALL;
    fwi.uCount = 4;
    fwi.dwTimeout = 0;
    FlashWindowEx(&fwi);
}

HWND BrowserView::GetNativeHandle()
{
    return GetWidget()->GetWindow()->GetNativeWindow();
}

void BrowserView::UpdateTitleBar()
{
    frame_->GetWindow()->UpdateWindowTitle();
    if(ShouldShowWindowIcon() && !loading_animation_timer_.IsRunning())
    {
        frame_->GetWindow()->UpdateWindowIcon();
    }
}

gfx::Rect BrowserView::GetRestoredBounds() const
{
    return frame_->GetWindow()->GetNormalBounds();
}

gfx::Rect BrowserView::GetBounds() const
{
    return frame_->GetWindow()->GetBounds();
}

bool BrowserView::IsMaximized() const
{
    return frame_->GetWindow()->IsMaximized();
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
    return frame_->GetWindow()->IsFullscreen();
}

bool BrowserView::IsFullscreenBubbleVisible() const
{
    return false;
}

void BrowserView::FocusAppMenu()
{
}

void BrowserView::RotatePaneFocus(bool forwards)
{
}

bool BrowserView::IsToolbarVisible() const
{
    return browser_->SupportsWindowFeature(Browser::FEATURE_TOOLBAR) ||
        browser_->SupportsWindowFeature(Browser::FEATURE_LOCATIONBAR);
}

void BrowserView::DisableInactiveFrame()
{
    frame_->GetWindow()->DisableInactiveRendering();
}

void BrowserView::UserChangedTheme()
{
    frame_->GetWindow()->FrameTypeChanged();
}

void BrowserView::DestroyBrowser()
{
    browser_.reset();
}

void BrowserView::ShowAppMenu()
{
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, NotificationObserver implementation:

void BrowserView::Observe(NotificationType type,
                          const NotificationSource& source,
                          const NotificationDetails& details)
{
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
}

bool BrowserView::GetAcceleratorForCommandId(int command_id,
                                             view::MenuAccelerator* accelerator)
{
    return false;
}

bool BrowserView::IsItemForCommandIdDynamic(int command_id) const
{
    return false;
}

string16 BrowserView::GetLabelForCommandId(int command_id) const
{
    return string16();
}

void BrowserView::ExecuteCommand(int command_id)
{
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, views::WindowDelegate implementation:

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
    return true;
}

bool BrowserView::IsModal() const
{
    return false;
}

std::wstring BrowserView::GetWindowTitle() const
{
    return std::wstring();
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
    if(browser_->type() & Browser::TYPE_APP)
    {
    }

    return GetWindowIcon();
}

SkBitmap BrowserView::GetWindowIcon()
{
    if(browser_->type() & Browser::TYPE_APP)
    {
    }
    return SkBitmap();
}

bool BrowserView::ShouldShowWindowIcon() const
{
    return browser_->SupportsWindowFeature(Browser::FEATURE_TITLEBAR);
}

bool BrowserView::ExecuteWindowsCommand(int command_id)
{
    // This function handles WM_SYSCOMMAND, WM_APPCOMMAND, and WM_COMMAND.

    // Translate WM_APPCOMMAND command ids into a command id that the browser
    // knows how to handle.
    int command_id_from_app_command = GetCommandIDForAppCommandID(command_id);
    if(command_id_from_app_command != -1)
    {
        command_id = command_id_from_app_command;
    }

    return false;
}

std::wstring BrowserView::GetWindowName() const
{
    return std::wstring();
}

bool BrowserView::GetSavedWindowBounds(gfx::Rect* bounds) const
{
    // We return true because we can _always_ locate reasonable bounds using the
    // WindowSizer, and we don't want to trigger the Window's built-in "size to
    // default" handling because the browser window has no default preferred
    // size.
    return true;
}

bool BrowserView::GetSavedMaximizedState(bool* maximized) const
{
    return true;
}

view::View* BrowserView::GetContentsView()
{
    return new view::View();
}

view::ClientView* BrowserView::CreateClientView(view::Window* window)
{
    set_window(window);
    return this;
}

void BrowserView::OnWindowActivationChanged(bool active)
{
}

void BrowserView::OnWindowBeginUserBoundsChange()
{
}

void BrowserView::OnWidgetMove()
{
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, views::ClientView overrides:

bool BrowserView::CanClose()
{
    // Empty TabStripModel, it's now safe to allow the Window to be closed.
    NotificationService::current()->Notify(
        NotificationType::WINDOW_CLOSED,
        Source<HWND>(frame_->GetWindow()->GetNativeWindow()),
        NotificationService::NoDetails());
    return true;
}

int BrowserView::NonClientHitTest(const gfx::Point& point)
{
    return GetBrowserViewLayout()->NonClientHitTest(point);
}

gfx::Size BrowserView::GetMinimumSize()
{
    return GetBrowserViewLayout()->GetMinimumSize();
}

bool BrowserView::SplitHandleMoved(view::SingleSplitView* view)
{
    for(int i=0; i<view->child_count(); ++i)
    {
        view->GetChildViewAt(i)->InvalidateLayout();
    }
    SchedulePaint();
    Layout();
    return false;
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

///////////////////////////////////////////////////////////////////////////////
// BrowserView, views::View overrides:

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

    // The status bubble position requires that all other layout finish first.
    LayoutStatusBubble();
}

void BrowserView::PaintChildren(gfx::Canvas* canvas)
{
    // Paint the |infobar_container_| last so that it may paint its
    // overlapping tabs.
    for(int i=0; i<child_count(); ++i)
    {
        View* child = GetChildViewAt(i);
        child->Paint(canvas);
    }
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

void BrowserView::GetAccessibleState(AccessibleViewState* state)
{
    state->name = view::GetStringUTF16(IDS_PRODUCT_NAME);
    state->role = AccessibilityTypes::ROLE_CLIENT;
}

view::LayoutManager* BrowserView::CreateLayoutManager() const
{
    return new BrowserViewLayout;
}

void BrowserView::Init()
{
    SetLayoutManager(CreateLayoutManager());
    // Stow a pointer to this object onto the window handle so that we can get at
    // it later when all we have is a native view.
    GetWidget()->native_widget()->SetNativeWindowProperty(kBrowserViewKey, this);

    // Stow a pointer to the browser's profile onto the window handle so that we
    // can get it later when all we have is a native view.
    GetWidget()->native_widget()->SetNativeWindowProperty(Profile::kProfileKey,
        browser_->profile());

    LoadAccelerators();

    // We're now initialized and ready to process Layout requests.
    ignore_layout_ = false;
}

// static
BrowserWindow* BrowserWindow::CreateBrowserWindow(Browser* browser)
{
    // Create the view and the frame. The frame will attach itself via the view
    // so we don't need to do anything with the pointer.
    BrowserView* view = new BrowserView(browser);
    BrowserFrame::Create(view, browser->profile());

    view->GetWindow()->non_client_view()->SetAccessibleName(
        view::GetStringUTF16(IDS_PRODUCT_NAME));

    return view;
}