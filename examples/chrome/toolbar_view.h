
#ifndef __toolbar_view_h__
#define __toolbar_view_h__

#pragma once

#include <set>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"

#include "ui_base/animation/slide_animation.h"
#include "ui_base/models/accelerator.h"
#include "ui_base/models/simple_menu_model.h"

#include "view/controls/button/menu_button.h"
#include "view/controls/button/image_button.h"
#include "view/controls/menu/menu.h"
#include "view/controls/menu/menu_wrapper.h"
#include "view/controls/menu/view_menu_delegate.h"
#include "view/view.h"

#include "accessible_pane_view.h"
#include "location_bar_view.h"

class Browser;

// The Browser Window's toolbar.
class ToolbarView : public AccessiblePaneView,
    public view::ViewMenuDelegate,
    public ui::AcceleratorProvider,
    public LocationBarView::Delegate,
    public view::ButtonListener
{
public:
    // The view class name.
    static char kViewClassName[];

    explicit ToolbarView(Browser* browser);
    virtual ~ToolbarView();

    // Create the contents of the Browser Toolbar
    void Init();

    // Updates the toolbar (and transitively the location bar) with the states of
    // the specified |tab|.  If |should_restore_state| is true, we're switching
    // (back?) to this tab and should restore any previous location bar state
    // (such as user editing) as well.
    void Update(TabContents* tab, bool should_restore_state);

    // Set focus to the toolbar with complete keyboard access, with the
    // focus initially set to the location bar. Focus will be restored
    // to the ViewStorage with id |view_storage_id| if the user escapes.
    void SetPaneFocusAndFocusLocationBar(int view_storage_id);

    // Set focus to the toolbar with complete keyboard access, with the
    // focus initially set to the app menu. Focus will be restored
    // to the ViewStorage with id |view_storage_id| if the user escapes.
    void SetPaneFocusAndFocusAppMenu(int view_storage_id);

    // Returns true if the app menu is focused.
    bool IsAppMenuFocused();

    // Add a listener to receive a callback when the menu opens.
    void AddMenuListener(view::MenuListener* listener);

    // Remove a menu listener.
    void RemoveMenuListener(view::MenuListener* listener);

    // Gets a bitmap with the icon for the app menu and any overlaid notification
    // badge.
    SkBitmap GetAppMenuIcon(view::CustomButton::ButtonState state);

    virtual bool GetAcceleratorInfo(int id, ui::Accelerator* accel);

    // Accessors...
    Browser* browser() const { return browser_; }
    LocationBarView* location_bar() const { return location_bar_; }
    view::MenuButton* app_menu() const { return app_menu_; }

    // Overridden from AccessiblePaneView
    virtual bool SetPaneFocus(int view_storage_id, View* initial_focus);
    virtual void GetAccessibleState(ui::AccessibleViewState* state);

    // Overridden from view::MenuDelegate:
    virtual void RunMenu(view::View* source, const gfx::Point& pt);

    // Overridden from LocationBarView::Delegate:
    virtual TabContentsWrapper* GetTabContentsWrapper() const;
    virtual void OnInputInProgress(bool in_progress);

    // Overridden from view::BaseButton::ButtonListener:
    virtual void ButtonPressed(view::Button* sender, const view::Event& event);

    // Overridden from ui::AcceleratorProvider:
    virtual bool GetAcceleratorForCommandId(
        int command_id, ui::Accelerator* accelerator);

    // Overridden from view::View:
    virtual gfx::Size GetPreferredSize();
    virtual void Layout();
    virtual void OnPaint(gfx::Canvas* canvas);
    virtual bool GetDropFormats(
        int* formats,
        std::set<ui::OSExchangeData::CustomFormat>* custom_formats);
    virtual bool CanDrop(const ui::OSExchangeData& data);
    virtual int OnDragUpdated(const view::DropTargetEvent& event);
    virtual int OnPerformDrop(const view::DropTargetEvent& event);
    virtual void OnThemeChanged();
    virtual std::string GetClassName() const;

    // The apparent horizontal space between most items, and the vertical padding
    // above and below them.
    static const int kStandardSpacing;
    // The top of the toolbar has an edge we have to skip over in addition to the
    // standard spacing.
    static const int kVertSpacing;

protected:

    // Overridden from AccessiblePaneView
    virtual view::View* GetDefaultFocusableChild();
    virtual void RemovePaneFocus();

private:
    // Returns true if we should show the upgrade recommended dot.
    bool IsUpgradeRecommended();

    // Returns true if we should show the background page badge.
    bool ShouldShowBackgroundPageBadge();

    // Returns true if we should show the warning for incompatible software.
    bool ShouldShowIncompatibilityWarning();

    // Returns the number of pixels above the location bar in non-normal display.
    int PopupTopSpacing() const;

    // Loads the images for all the child views.
    void LoadImages();

    // Types of display mode this toolbar can have.
    enum DisplayMode
    {
        DISPLAYMODE_NORMAL,       // Normal toolbar with buttons, etc.
        DISPLAYMODE_LOCATION      // Slimline toolbar showing only compact location
                                  // bar, used for popups.
    };
    bool IsDisplayModeNormal() const
    {
        return display_mode_ == DISPLAYMODE_NORMAL;
    }

    // Updates the badge on the app menu (Wrench).
    void UpdateAppMenuBadge();

    // Gets a badge for the wrench icon corresponding to the number of
    // unacknowledged background pages in the system.
    SkBitmap GetBackgroundPageBadge();

    // The model that contains the security level, text, icon to display...
    ToolbarModel* model_;

    // Controls
	view::ImageButton* new_session_btn_;
    view::ImageButton* dup_session_btn_;
    view::ImageButton* reload_session_btn_;
	view::ImageButton* session_setting_btn_;
	view::ImageButton* copy_all_btn_;
    view::ImageButton* paste_btn_;
	view::ImageButton* clear_btn_;
	view::ImageButton* log_enabler_btn_;
	view::ImageButton* shortcut_enabler_btn_;
    LocationBarView* location_bar_;
    view::MenuButton* app_menu_;
    Browser* browser_;

    // Contents of the profiles menu to populate with profile names.
    scoped_ptr<ui::SimpleMenuModel> profiles_menu_contents_;

    // The display mode used when laying out the toolbar.
    DisplayMode display_mode_;

    // The contents of the wrench menu.
    scoped_ptr<ui::SimpleMenuModel> wrench_menu_model_;

    // Vector of listeners to receive callbacks when the menu opens.
    std::vector<view::MenuListener*> menu_listeners_;

    DISALLOW_IMPLICIT_CONSTRUCTORS(ToolbarView);
};

#endif //__toolbar_view_h__