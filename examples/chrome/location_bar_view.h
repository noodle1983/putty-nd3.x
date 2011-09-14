
#ifndef __location_bar_view_h__
#define __location_bar_view_h__

#pragma once

#include <string>
#include <vector>

#include "base/task.h"

#include "ui_gfx/font.h"
#include "ui_gfx/rect.h"

#include "view/controls/native/native_view_host.h"
#include "view/drag_controller.h"

#include "dropdown_bar_host_delegate.h"
#include "location_bar.h"
#include "toolbar_model.h"
#include "window_open_disposition.h"

namespace view
{
    class HorizontalPainter;
    class Label;
}

class Browser;
class LocationIconView;
class TabContents;
class TabContentsWrapper;
class Url;

/////////////////////////////////////////////////////////////////////////////
//
// LocationBarView class
//
//   The LocationBarView class is a View subclass that paints the background
//   of the URL bar strip and contains its content.
//
/////////////////////////////////////////////////////////////////////////////
class LocationBarView : public LocationBar,
    public view::View,
    public view::DragController,
    public DropdownBarHostDelegate
{
public:
    // The location bar view's class name.
    static const char kViewClassName[];

    // DropdownBarHostDelegate
    virtual void SetFocusAndSelection(bool select_all) OVERRIDE;
    virtual void SetAnimationOffset(int offset) OVERRIDE;

    // Returns the offset used while animating.
    int animation_offset() const { return animation_offset_; }

    class Delegate
    {
    public:
        // Should return the current tab contents.
        virtual TabContentsWrapper* GetTabContentsWrapper() const = 0;

        // Called by the location bar view when the user starts typing in the edit.
        // This forces our security style to be UNKNOWN for the duration of the
        // editing.
        virtual void OnInputInProgress(bool in_progress) = 0;
    };

    enum ColorKind
    {
        BACKGROUND = 0,
        TEXT,
        SELECTED_TEXT,
        DEEMPHASIZED_TEXT,
        SECURITY_TEXT,
    };

    // The modes reflect the different scenarios where a location bar can be used.
    // The normal mode is the mode used in a regular browser window.
    // In popup mode, the location bar view is read only and has a slightly
    // different presentation (font size / color).
    // In app launcher mode, the location bar is empty and no security states or
    // page/browser actions are displayed.
    enum Mode
    {
        NORMAL = 0,
        POPUP,
        APP_LAUNCHER
    };

    LocationBarView(Browser* browser,
        ToolbarModel* model,
        Delegate* delegate,
        Mode mode);
    virtual ~LocationBarView();

    void Init();

    // True if this instance has been initialized by calling Init, which can only
    // be called when the receiving instance is attached to a view container.
    bool IsInitialized() const;

    // Returns the appropriate color for the desired kind, based on the user's
    // system theme.
    static SkColor GetColor(ColorKind kind);

    // Updates the location bar.  We also reset the bar's permanent text and
    // security style, and, if |tab_for_state_restoring| is non-NULL, also restore
    // saved state that the tab holds.
    void Update(const TabContents* tab_for_state_restoring);

    Browser* browser() const { return browser_; }

    // Toggles the star on or off.
    void SetStarToggled(bool on);

    // Shows the bookmark bubble.
    void ShowStarBubble(const Url& url, bool newly_bookmarked);

    // Returns the screen coordinates of the location entry (where the URL text
    // appears, not where the icons are shown).
    gfx::Point GetLocationEntryOrigin() const;

    // Sizing functions
    virtual gfx::Size GetPreferredSize() OVERRIDE;

    // Layout and Painting functions
    virtual void Layout() OVERRIDE;
    virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;

    // No focus border for the location bar, the caret is enough.
    virtual void OnPaintFocusBorder(gfx::Canvas* canvas) {}

    // Set if we should show a focus rect while the location entry field is
    // focused. Used when the toolbar is in full keyboard accessibility mode.
    // Repaints if necessary.
    virtual void SetShowFocusRect(bool show);

    // Select all of the text. Needed when the user tabs through controls
    // in the toolbar in full keyboard accessibility mode.
    virtual void SelectAll();

    // Event Handlers
    virtual bool OnMousePressed(const view::MouseEvent& event) OVERRIDE;
    virtual bool OnMouseDragged(const view::MouseEvent& event) OVERRIDE;
    virtual void OnMouseReleased(const view::MouseEvent& event) OVERRIDE;
    virtual void OnMouseCaptureLost() OVERRIDE;

    const LocationIconView* location_icon_view() const
    {
        return location_icon_view_;
    }

    // Overridden from view::View:
    virtual std::string GetClassName() const OVERRIDE;
    virtual bool SkipDefaultKeyEventProcessing(
        const view::KeyEvent& event) OVERRIDE;
    virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

    // Overridden from view::DragController:
    virtual void WriteDragDataForView(View* sender,
        const gfx::Point& press_pt,
        ui::OSExchangeData* data) OVERRIDE;
    virtual int GetDragOperationsForView(View* sender,
        const gfx::Point& p) OVERRIDE;
    virtual bool CanStartDragForView(View* sender,
        const gfx::Point& press_pt,
        const gfx::Point& p) OVERRIDE;

    // Overridden from LocationBar:
    virtual void SetSuggestedText(const string16& text) OVERRIDE;
    virtual string16 GetInputString() const OVERRIDE;
    virtual WindowOpenDisposition GetWindowOpenDisposition() const OVERRIDE;
    virtual void AcceptInput() OVERRIDE;
    virtual void FocusLocation(bool select_all) OVERRIDE;
    virtual void FocusSearch() OVERRIDE;
    virtual void UpdateContentSettingsIcons() OVERRIDE;
    virtual void UpdatePageActions() OVERRIDE;
    virtual void InvalidatePageActions() OVERRIDE;
    virtual void SaveStateToContents(TabContents* contents) OVERRIDE;
    virtual void Revert() OVERRIDE;
    virtual const OmniboxView* location_entry() const OVERRIDE;
    virtual OmniboxView* location_entry() OVERRIDE;

    // Thickness of the left and right edges of the omnibox, in normal mode.
    static const int kNormalHorizontalEdgeThickness;
    // Thickness of the top and bottom edges of the omnibox.
    static const int kVerticalEdgeThickness;
    // Space between items in the location bar.
    static const int kItemPadding;
    // Amount of padding built into the standard omnibox icons.
    static const int kIconInternalPadding;
    // Space between the edges and the items next to them.
    static const int kEdgeItemPadding;
    // Space between the edge and a bubble.
    static const int kBubbleHorizontalPadding;

protected:
    virtual void OnFocus() OVERRIDE;

private:
    // Returns the amount of horizontal space (in pixels) out of
    // |location_bar_width| that is not taken up by the actual text in
    // location_entry_.
    int AvailableWidth(int location_bar_width);

    // If |view| fits in |available_width|, it is made visible and positioned at
    // the leading or trailing end of |bounds|, which are then shrunk
    // appropriately.  Otherwise |view| is made invisible.
    // Note: |view| is expected to have already been positioned and sized
    // vertically.
    void LayoutView(view::View* view,
        int padding,
        int available_width,
        bool leading,
        gfx::Rect* bounds);

    // Update the visibility state of the Content Blocked icons to reflect what is
    // actually blocked on the current page.
    void RefreshContentSettingViews();

    // Sets the visibility of view to new_vis.
    void ToggleVisibility(bool new_vis, view::View* view);

    // Helper for the Mouse event handlers that does all the real work.
    void OnMouseEvent(const view::MouseEvent& event, UINT msg);

    // Returns true if the suggest text is valid.
    bool HasValidSuggestText() const;

    // The Autocomplete Edit field.
    //scoped_ptr<OmniboxView> location_entry_;

    // The Browser object that corresponds to this View.
    Browser* browser_;

    // The model.
    ToolbarModel* model_;

    // Our delegate.
    Delegate* delegate_;

    // This is the string of text from the autocompletion session that the user
    // entered or selected.
    string16 location_input_;

    // The user's desired disposition for how their input should be opened
    WindowOpenDisposition disposition_;

    // Font used by edit and some of the hints.
    gfx::Font font_;

    // An object used to paint the normal-mode background.
    scoped_ptr<view::HorizontalPainter> painter_;

    // An icon to the left of the edit field.
    LocationIconView* location_icon_view_;

    // Location_entry view
    view::View* location_entry_view_;

    // The mode that dictates how the bar shows.
    Mode mode_;

    // True if we should show a focus rect while the location entry field is
    // focused. Used when the toolbar is in full keyboard accessibility mode.
    bool show_focus_rect_;

    // While animating, the host clips the widget and draws only the bottom
    // part of it. The view needs to know the pixel offset at which we are drawing
    // the widget so that we can draw the curved edges that attach to the toolbar
    // in the right location.
    int animation_offset_;

    DISALLOW_IMPLICIT_CONSTRUCTORS(LocationBarView);
};

#endif //__location_bar_view_h__