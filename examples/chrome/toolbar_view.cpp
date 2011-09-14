
#include "toolbar_view.h"

#include "base/utf_string_conversions.h"

#include "ui_gfx/canvas_skia.h"

#include "ui_base/accessibility/accessible_view_state.h"
#include "ui_base/dragdrop/drag_drop_types.h"
#include "ui_base/dragdrop/os_exchange_data.h"
#include "ui_base/l10n/l10n_util.h"
#include "ui_base/resource/resource_bundle.h"
#include "ui_base/theme_provider.h"

#include "view/controls/button/button_dropdown.h"
#include "view/widget/widget.h"

#include "../wanui_res/resource.h"

#include "browser.h"
#include "browser_window.h"
#include "chrome_command_ids.h"
#include "view_ids.h"

// static
char ToolbarView::kViewClassName[] = "browser/ToolbarView";
// The space between items is 4 px in general.
const int ToolbarView::kStandardSpacing = 4;
// The top of the toolbar has an edge we have to skip over in addition to the 4
// px of spacing.
const int ToolbarView::kVertSpacing = kStandardSpacing + 1;
// The edge graphics have some built-in spacing/shadowing, so we have to adjust
// our spacing to make it still appear to be 4 px.
static const int kEdgeSpacing = ToolbarView::kStandardSpacing - 1;
// The buttons to the left of the omnibox are close together.
static const int kButtonSpacing = 1;

static const int kStatusBubbleWidth = 480;

// The length of time to run the upgrade notification animation (the time it
// takes one pulse to run its course and go back to its original brightness).
static const int kPulseDuration = 2000;

// How long to wait between pulsating the upgrade notifier.
static const int kPulsateEveryMs = 8000;

static const int kPopupTopSpacingNonGlass = 3;
static const int kPopupBottomSpacingNonGlass = 2;
static const int kPopupBottomSpacingGlass = 1;

// Top margin for the wrench menu badges (badge is placed in the upper right
// corner of the wrench menu).
static const int kBadgeTopMargin = 2;

static SkBitmap* kPopupBackgroundEdge = NULL;

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, public:

ToolbarView::ToolbarView(Browser* browser)
: model_(browser->toolbar_model()),
back_(NULL),
forward_(NULL),
home_(NULL),
location_bar_(NULL),
app_menu_(NULL),
browser_(browser),
profiles_menu_contents_(NULL)
{
    set_id(VIEW_ID_TOOLBAR);

    display_mode_ = browser->SupportsWindowFeature(Browser::FEATURE_TABSTRIP) ?
        DISPLAYMODE_NORMAL : DISPLAYMODE_LOCATION;

    if(!kPopupBackgroundEdge)
    {
        kPopupBackgroundEdge = ui::ResourceBundle::GetSharedInstance().GetBitmapNamed(
            IDR_LOCATIONBG_POPUPMODE_EDGE);
    }
}

ToolbarView::~ToolbarView()
{
    // NOTE: Don't remove the command observers here.  This object gets destroyed
    // after the Browser (which owns the CommandUpdater), so the CommandUpdater is
    // already gone.
}

void ToolbarView::Init()
{
    //back_menu_model_.reset(new BackForwardMenuModel(
    //    browser_, BackForwardMenuModel::BACKWARD_MENU));
    //forward_menu_model_.reset(new BackForwardMenuModel(
    //    browser_, BackForwardMenuModel::FORWARD_MENU));
    //wrench_menu_model_.reset(new WrenchMenuModel(this, browser_));
    back_ = new view::ButtonDropDown(this, NULL/*back_menu_model_.get()*/);
    back_->set_triggerable_event_flags(ui::EF_LEFT_BUTTON_DOWN |
        ui::EF_MIDDLE_BUTTON_DOWN);
    back_->set_tag(IDC_BACK);
    back_->SetImageAlignment(view::ImageButton::ALIGN_RIGHT,
        view::ImageButton::ALIGN_TOP);
    back_->SetTooltipText(
        UTF16ToWide(ui::GetStringUTF16(IDS_TOOLTIP_BACK)));
    back_->SetAccessibleName(ui::GetStringUTF16(IDS_ACCNAME_BACK));
    back_->set_id(VIEW_ID_BACK_BUTTON);

    forward_ = new view::ButtonDropDown(this, NULL/*forward_menu_model_.get()*/);
    forward_->set_triggerable_event_flags(ui::EF_LEFT_BUTTON_DOWN |
        ui::EF_MIDDLE_BUTTON_DOWN);
    forward_->set_tag(IDC_FORWARD);
    forward_->SetTooltipText(
        UTF16ToWide(ui::GetStringUTF16(IDS_TOOLTIP_FORWARD)));
    forward_->SetAccessibleName(ui::GetStringUTF16(IDS_ACCNAME_FORWARD));
    forward_->set_id(VIEW_ID_FORWARD_BUTTON);

    // Have to create this before |reload_| as |reload_|'s constructor needs it.
    location_bar_ = new LocationBarView(browser_, model_, this,
        (display_mode_ == DISPLAYMODE_LOCATION) ?
        LocationBarView::POPUP : LocationBarView::NORMAL);

    //reload_ = new ReloadButton(location_bar_, browser_);
    //reload_->set_triggerable_event_flags(ui::EF_LEFT_BUTTON_DOWN |
    //    ui::EF_MIDDLE_BUTTON_DOWN);
    //reload_->set_tag(IDC_RELOAD);
    //reload_->SetTooltipText(
    //    UTF16ToWide(ui::GetStringUTF16(IDS_TOOLTIP_RELOAD)));
    //reload_->SetAccessibleName(ui::GetStringUTF16(IDS_ACCNAME_RELOAD));
    //reload_->set_id(VIEW_ID_RELOAD_BUTTON);

    home_ = new view::ImageButton(this);
    home_->set_triggerable_event_flags(ui::EF_LEFT_BUTTON_DOWN |
        ui::EF_MIDDLE_BUTTON_DOWN);
    home_->set_tag(IDC_HOME);
    home_->SetTooltipText(
        UTF16ToWide(ui::GetStringUTF16(IDS_TOOLTIP_HOME)));
    home_->SetAccessibleName(ui::GetStringUTF16(IDS_ACCNAME_HOME));
    home_->set_id(VIEW_ID_HOME_BUTTON);

    //app_menu_ = new AppMenuButtonWin(this);
    //app_menu_->set_border(NULL);
    //app_menu_->EnableCanvasFlippingForRTLUI(true);
    //app_menu_->SetAccessibleName(ui::GetStringUTF16(IDS_ACCNAME_APP));
    //app_menu_->SetTooltipText(UTF16ToWide(ui::GetStringFUTF16(
    //    IDS_APPMENU_TOOLTIP,
    //    ui::GetStringUTF16(IDS_PRODUCT_NAME))));
    //app_menu_->set_id(VIEW_ID_APP_MENU);

    // Add any necessary badges to the menu item based on the system state.
    if(IsUpgradeRecommended() || ShouldShowIncompatibilityWarning())
    {
        UpdateAppMenuBadge();
    }
    LoadImages();

    // Always add children in order from left to right, for accessibility.
    AddChildView(back_);
    AddChildView(forward_);
    //AddChildView(reload_);
    AddChildView(home_);
    AddChildView(location_bar_);
    //AddChildView(browser_actions_);
    //AddChildView(app_menu_);

    location_bar_->Init();
}

void ToolbarView::Update(TabContents* tab, bool should_restore_state)
{
    if(location_bar_)
    {
        location_bar_->Update(should_restore_state ? tab : NULL);
    }
}

void ToolbarView::SetPaneFocusAndFocusLocationBar(int view_storage_id)
{
    SetPaneFocus(view_storage_id, location_bar_);
}

void ToolbarView::SetPaneFocusAndFocusAppMenu(int view_storage_id)
{
    SetPaneFocus(view_storage_id, app_menu_);
}

bool ToolbarView::IsAppMenuFocused()
{
    return app_menu_->HasFocus();
}

void ToolbarView::AddMenuListener(view::MenuListener* listener)
{
    menu_listeners_.push_back(listener);
}

void ToolbarView::RemoveMenuListener(view::MenuListener* listener)
{
    for(std::vector<view::MenuListener*>::iterator i(menu_listeners_.begin());
        i!=menu_listeners_.end(); ++i)
    {
        if(*i == listener)
        {
            menu_listeners_.erase(i);
            return;
        }
    }
}

SkBitmap ToolbarView::GetAppMenuIcon(view::CustomButton::ButtonState state)
{
    ui::ThemeProvider* tp = GetThemeProvider();

    int id = 0;
    switch(state)
    {
    case view::CustomButton::BS_NORMAL: id = IDR_TOOLS;   break;
    case view::CustomButton::BS_HOT:    id = IDR_TOOLS_H; break;
    case view::CustomButton::BS_PUSHED: id = IDR_TOOLS_P; break;
    default:                            NOTREACHED();     break;
    }
    SkBitmap icon = *tp->GetBitmapNamed(id);

    // Keep track of whether we were showing the badge before, so we don't send
    // multiple UMA events for example when multiple Chrome windows are open.
    static bool incompatibility_badge_showing = false;
    // Save the old value before resetting it.
    bool was_showing = incompatibility_badge_showing;
    incompatibility_badge_showing = false;

    bool add_badge = IsUpgradeRecommended() || ShouldShowIncompatibilityWarning();
    if(!add_badge)
    {
        return icon;
    }

    // Draw the chrome app menu icon onto the canvas.
    scoped_ptr<gfx::CanvasSkia> canvas(
        new gfx::CanvasSkia(icon.width(), icon.height(), false));
    canvas->DrawBitmapInt(icon, 0, 0);

    SkBitmap badge;
    // Only one badge can be active at any given time. The Upgrade notification
    // is deemed most important, then the DLL conflict badge.
    if(IsUpgradeRecommended())
    {
        //badge = *tp->GetBitmapNamed(
        //    UpgradeDetector::GetInstance()->GetIconResourceID(
        //    UpgradeDetector::UPGRADE_ICON_TYPE_BADGE));
    }
    else if(ShouldShowIncompatibilityWarning())
    {
        //badge = *tp->GetBitmapNamed(IDR_CONFLICT_BADGE);
        //incompatibility_badge_showing = true;
    }
    else
    {
        NOTREACHED();
    }

    canvas->DrawBitmapInt(badge, icon.width() - badge.width(), kBadgeTopMargin);

    return canvas->ExtractBitmap();
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, AccessiblePaneView overrides:

bool ToolbarView::SetPaneFocus(int view_storage_id, view::View* initial_focus)
{
    if(!AccessiblePaneView::SetPaneFocus(view_storage_id, initial_focus))
    {
        return false;
    }

    location_bar_->SetShowFocusRect(true);
    return true;
}

void ToolbarView::GetAccessibleState(ui::AccessibleViewState* state)
{
    state->role = ui::AccessibilityTypes::ROLE_TOOLBAR;
    state->name = ui::GetStringUTF16(IDS_ACCNAME_TOOLBAR);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, Menu::Delegate overrides:

bool ToolbarView::GetAcceleratorInfo(int id, ui::Accelerator* accel)
{
    return GetWidget()->GetAccelerator(id, accel);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, view::MenuDelegate implementation:

void ToolbarView::RunMenu(view::View* source, const gfx::Point& /* pt */)
{
    DCHECK_EQ(VIEW_ID_APP_MENU, source->id());

    //wrench_menu_ = new WrenchMenu(browser_);
    //wrench_menu_->Init(wrench_menu_model_.get());

    //for(size_t i=0; i<menu_listeners_.size(); ++i)
    //{
    //    menu_listeners_[i]->OnMenuOpened();
    //}

    //wrench_menu_->RunMenu(app_menu_);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, LocationBarView::Delegate implementation:

TabContentsWrapper* ToolbarView::GetTabContentsWrapper() const
{
    return browser_->GetSelectedTabContentsWrapper();
}

void ToolbarView::OnInputInProgress(bool in_progress)
{
    // The edit should make sure we're only notified when something changes.
    DCHECK(model_->input_in_progress() != in_progress);

    model_->set_input_in_progress(in_progress);
    location_bar_->Update(NULL);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, view::Button::ButtonListener implementation:

void ToolbarView::ButtonPressed(view::Button* sender,
                                const view::Event& event)
{
    //int command = sender->tag();
    //WindowOpenDisposition disposition =
    //    event_utils::DispositionFromEventFlags(sender->mouse_event_flags());
    //if((disposition == CURRENT_TAB) &&
    //    ((command == IDC_BACK) || (command == IDC_FORWARD)))
    //{
    //    // Forcibly reset the location bar, since otherwise it won't discard any
    //    // ongoing user edits, since it doesn't realize this is a user-initiated
    //    // action.
    //    location_bar_->Revert();
    //}
    //browser_->ExecuteCommandWithDisposition(command, disposition);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, ui::AcceleratorProvider implementation:

bool ToolbarView::GetAcceleratorForCommandId(int command_id,
                                             ui::Accelerator* accelerator)
{
    // The standard Ctrl-X, Ctrl-V and Ctrl-C are not defined as accelerators
    // anywhere so we need to check for them explicitly here.
    // TODO(cpu) Bug 1109102. Query WebKit land for the actual bindings.
    switch(command_id)
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
    // Else, we retrieve the accelerator information from the frame.
    return GetWidget()->GetAccelerator(command_id, accelerator);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, view::View overrides:

gfx::Size ToolbarView::GetPreferredSize()
{
    if(IsDisplayModeNormal())
    {
        int min_width = kEdgeSpacing +
            back_->GetPreferredSize().width() + kButtonSpacing +
            forward_->GetPreferredSize().width() + kButtonSpacing +
            /*reload_->GetPreferredSize().width() + kStandardSpacing +
            (show_home_button_.GetValue() ?
            (home_->GetPreferredSize().width() + kButtonSpacing) : 0) +*/
            location_bar_->GetPreferredSize().width() /*+
            browser_actions_->GetPreferredSize().width() +
            app_menu_->GetPreferredSize().width() + kEdgeSpacing*/;

        static SkBitmap normal_background;
        if(normal_background.isNull())
        {
            ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
            normal_background = *rb.GetBitmapNamed(IDR_CONTENT_TOP_CENTER);
        }

        return gfx::Size(min_width, normal_background.height());
    }

    int vertical_spacing = PopupTopSpacing() +
        (GetWidget()->ShouldUseNativeFrame() ?
        kPopupBottomSpacingGlass : kPopupBottomSpacingNonGlass);
    return gfx::Size(0, location_bar_->GetPreferredSize().height() +
        vertical_spacing);
}

void ToolbarView::Layout()
{
    // If we have not been initialized yet just do nothing.
    if(back_ == NULL)
    {
        return;
    }

    bool maximized = browser_->window() && browser_->window()->IsMaximized();
    if(!IsDisplayModeNormal())
    {
        int edge_width = maximized ?
            0 : kPopupBackgroundEdge->width(); // See OnPaint().
        location_bar_->SetBounds(edge_width, PopupTopSpacing(),
            width() - (edge_width * 2), location_bar_->GetPreferredSize().height());
        return;
    }

    int child_y = std::min(kVertSpacing, height());
    // We assume all child elements are the same height.
    int child_height =
        std::min(back_->GetPreferredSize().height(), height() - child_y);

    // If the window is maximized, we extend the back button to the left so that
    // clicking on the left-most pixel will activate the back button.
    // TODO(abarth):  If the window becomes maximized but is not resized,
    //                then Layout() might not be called and the back button
    //                will be slightly the wrong size.  We should force a
    //                Layout() in this case.
    //                http://crbug.com/5540
    int back_width = back_->GetPreferredSize().width();
    if(maximized)
    {
        back_->SetBounds(0, child_y, back_width + kEdgeSpacing, child_height);
    }
    else
    {
        back_->SetBounds(kEdgeSpacing, child_y, back_width, child_height);
    }

    forward_->SetBounds(back_->x() + back_->width() + kButtonSpacing,
        child_y, forward_->GetPreferredSize().width(), child_height);

    //reload_->SetBounds(forward_->x() + forward_->width() + kButtonSpacing,
    //    child_y, reload_->GetPreferredSize().width(), child_height);

    //if(show_home_button_.GetValue())
    //{
    //    home_->SetVisible(true);
    //    home_->SetBounds(reload_->x() + reload_->width() + kButtonSpacing, child_y,
    //        home_->GetPreferredSize().width(), child_height);
    //}
    //else
    //{
    //    home_->SetVisible(false);
    //    home_->SetBounds(reload_->x() + reload_->width(), child_y, 0, child_height);
    //}

    //int browser_actions_width = browser_actions_->GetPreferredSize().width();
    //int app_menu_width = app_menu_->GetPreferredSize().width();
    //int location_x = home_->x() + home_->width() + kStandardSpacing;
    //int available_width = width() - kEdgeSpacing - app_menu_width -
    //    browser_actions_width - location_x;

    //location_bar_->SetBounds(location_x, child_y, std::max(available_width, 0),
    //    child_height);

    //browser_actions_->SetBounds(location_bar_->x() + location_bar_->width(), 0,
    //    browser_actions_width, height());
    //// The browser actions need to do a layout explicitly, because when an
    //// extension is loaded/unloaded/changed, BrowserActionContainer removes and
    //// re-adds everything, regardless of whether it has a page action. For a
    //// page action, browser action bounds do not change, as a result of which
    //// SetBounds does not do a layout at all.
    //// TODO(sidchat): Rework the above behavior so that explicit layout is not
    ////                required.
    //browser_actions_->Layout();

    //// Extend the app menu to the screen's right edge in maximized mode just like
    //// we extend the back button to the left edge.
    //if(maximized)
    //{
    //    app_menu_width += kEdgeSpacing;
    //}
    //app_menu_->SetBounds(browser_actions_->x() + browser_actions_width, child_y,
    //    app_menu_width, child_height);
}

void ToolbarView::OnPaint(gfx::Canvas* canvas)
{
    View::OnPaint(canvas);

    if(IsDisplayModeNormal())
    {
        return;
    }

    // In maximized mode, we don't draw the endcaps on the location bar, because
    // when they're flush against the edge of the screen they just look glitchy.
    if(!browser_->window() || !browser_->window()->IsMaximized())
    {
        int top_spacing = PopupTopSpacing();
        canvas->DrawBitmapInt(*kPopupBackgroundEdge, 0, top_spacing);
        canvas->DrawBitmapInt(*kPopupBackgroundEdge,
            width() - kPopupBackgroundEdge->width(), top_spacing);
    }

    // For glass, we need to draw a black line below the location bar to separate
    // it from the content area.  For non-glass, the NonClientView draws the
    // toolbar background below the location bar for us.
    // NOTE: Keep this in sync with BrowserView::GetInfoBarSeparatorColor()!
    if(GetWidget()->ShouldUseNativeFrame())
    {
        canvas->FillRectInt(SK_ColorBLACK, 0, height() - 1, width(), 1);
    }
}

// Note this method is ignored on Windows, but needs to be implemented for
// linux, where it is called before CanDrop().
bool ToolbarView::GetDropFormats(int* formats,
                                 std::set<ui::OSExchangeData::CustomFormat>* custom_formats)
{
    *formats = /*ui::OSExchangeData::URL | */ui::OSExchangeData::STRING;
    return true;
}

bool ToolbarView::CanDrop(const ui::OSExchangeData& data)
{
    // To support loading URLs by dropping into the toolbar, we need to support
    // dropping URLs and/or text.
    return /*data.HasURL() || */data.HasString();
}

int ToolbarView::OnDragUpdated(const view::DropTargetEvent& event)
{
    if(event.source_operations() & ui::DragDropTypes::DRAG_COPY)
    {
        return ui::DragDropTypes::DRAG_COPY;
    }
    else if(event.source_operations() & ui::DragDropTypes::DRAG_LINK)
    {
        return ui::DragDropTypes::DRAG_LINK;
    }
    return ui::DragDropTypes::DRAG_NONE;
}

int ToolbarView::OnPerformDrop(const view::DropTargetEvent& event)
{
    return ui::DragDropTypes::DRAG_NONE;
    //return location_bar_->location_entry()->OnPerformDrop(event);
}

void ToolbarView::OnThemeChanged()
{
    LoadImages();
}

std::string ToolbarView::GetClassName() const
{
    return kViewClassName;
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, protected:

// Override this so that when the user presses F6 to rotate toolbar panes,
// the location bar gets focus, not the first control in the toolbar.
view::View* ToolbarView::GetDefaultFocusableChild()
{
    return location_bar_;
}

void ToolbarView::RemovePaneFocus()
{
    AccessiblePaneView::RemovePaneFocus();
    location_bar_->SetShowFocusRect(false);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, private:

bool ToolbarView::IsUpgradeRecommended()
{
    return false;
    //return (UpgradeDetector::GetInstance()->notify_upgrade());
}

bool ToolbarView::ShouldShowIncompatibilityWarning()
{
    return false;
    //EnumerateModulesModel* loaded_modules = EnumerateModulesModel::GetInstance();
    //return loaded_modules->ShouldShowConflictWarning();
}

int ToolbarView::PopupTopSpacing() const
{
    // TODO(beng): For some reason GetWidget() returns NULL here in some
    //             unidentified circumstances on ChromeOS. This means GetWidget()
    //             succeeded but we were (probably) unable to locate a
    //             NativeWidgetGtk* on it using
    //             NativeWidget::GetNativeWidgetForNativeView.
    //             I am throwing in a NULL check for now to stop the hurt, but
    //             it's possible the crash may just show up somewhere else.
    const view::Widget* widget = GetWidget();
    DCHECK(widget) << "If you hit this please talk to beng";
    return widget && widget->ShouldUseNativeFrame() ?
        0 : kPopupTopSpacingNonGlass;
}

void ToolbarView::LoadImages()
{
    ui::ThemeProvider* tp = GetThemeProvider();

    back_->SetImage(view::CustomButton::BS_NORMAL, tp->GetBitmapNamed(IDR_BACK));
    back_->SetImage(view::CustomButton::BS_HOT, tp->GetBitmapNamed(IDR_BACK_H));
    back_->SetImage(view::CustomButton::BS_PUSHED,
        tp->GetBitmapNamed(IDR_BACK_P));
    back_->SetImage(view::CustomButton::BS_DISABLED,
        tp->GetBitmapNamed(IDR_BACK_D));

    forward_->SetImage(view::CustomButton::BS_NORMAL,
        tp->GetBitmapNamed(IDR_FORWARD));
    forward_->SetImage(view::CustomButton::BS_HOT,
        tp->GetBitmapNamed(IDR_FORWARD_H));
    forward_->SetImage(view::CustomButton::BS_PUSHED,
        tp->GetBitmapNamed(IDR_FORWARD_P));
    forward_->SetImage(view::CustomButton::BS_DISABLED,
        tp->GetBitmapNamed(IDR_FORWARD_D));

    home_->SetImage(view::CustomButton::BS_NORMAL, tp->GetBitmapNamed(IDR_HOME));
    home_->SetImage(view::CustomButton::BS_HOT, tp->GetBitmapNamed(IDR_HOME_H));
    home_->SetImage(view::CustomButton::BS_PUSHED, tp->GetBitmapNamed(IDR_HOME_P));

    //app_menu_->SetIcon(GetAppMenuIcon(view::CustomButton::BS_NORMAL));
    //app_menu_->SetHoverIcon(GetAppMenuIcon(view::CustomButton::BS_HOT));
    //app_menu_->SetPushedIcon(GetAppMenuIcon(view::CustomButton::BS_PUSHED));
}

void ToolbarView::UpdateAppMenuBadge()
{
    app_menu_->SetIcon(GetAppMenuIcon(view::CustomButton::BS_NORMAL));
    app_menu_->SetHoverIcon(GetAppMenuIcon(view::CustomButton::BS_HOT));
    app_menu_->SetPushedIcon(GetAppMenuIcon(view::CustomButton::BS_PUSHED));
    SchedulePaint();
}