
#include "location_bar_view.h"

#include "base/command_line.h"
#include "base/stl_utilinl.h"
#include "base/utf_string_conversions.h"

#include "ui_gfx/canvas_skia.h"
#include "ui_gfx/color_utils.h"
#include "ui_gfx/skia_util.h"

#include "ui_base/accessibility/accessible_view_state.h"
#include "ui_base/dragdrop/drag_drop_types.h"
#include "ui_base/l10n/l10n_util.h"
#include "ui_base/resource/resource_bundle.h"
#include "ui_base/theme_provider.h"

#include "view/controls/label.h"
#include "view/drag_utils.h"
#include "view/painter.h"
#include "view/widget/widget.h"

#include "../bubble/bubble_border.h"

#include "../wanui_res/resource.h"

#include "location_icon_view.h"
#include "tab_contents.h"
#include "tab_contents_wrapper.h"
#include "view_ids.h"

using view::View;

namespace
{

    TabContents* GetTabContentsFromDelegate(LocationBarView::Delegate* delegate)
    {
        const TabContentsWrapper* wrapper = delegate->GetTabContentsWrapper();
        return wrapper ? wrapper->tab_contents() : NULL;
    }

}

// static
const int LocationBarView::kNormalHorizontalEdgeThickness = 1;
const int LocationBarView::kVerticalEdgeThickness = 2;
const int LocationBarView::kItemPadding = 3;
const int LocationBarView::kIconInternalPadding = 2;
const int LocationBarView::kEdgeItemPadding = kItemPadding;
const int LocationBarView::kBubbleHorizontalPadding = 1;
const char LocationBarView::kViewClassName[] = "browser/LocationBarView";

static const int kEVBubbleBackgroundImages[] =
{
    IDR_OMNIBOX_EV_BUBBLE_BACKGROUND_L,
    IDR_OMNIBOX_EV_BUBBLE_BACKGROUND_C,
    IDR_OMNIBOX_EV_BUBBLE_BACKGROUND_R,
};

static const int kSelectedKeywordBackgroundImages[] =
{
    IDR_LOCATION_BAR_SELECTED_KEYWORD_BACKGROUND_L,
    IDR_LOCATION_BAR_SELECTED_KEYWORD_BACKGROUND_C,
    IDR_LOCATION_BAR_SELECTED_KEYWORD_BACKGROUND_R,
};

static const int kNormalModeBackgroundImages[] =
{
    IDR_LOCATIONBG_L,
    IDR_LOCATIONBG_C,
    IDR_LOCATIONBG_R,
};

// LocationBarView -----------------------------------------------------------

LocationBarView::LocationBarView(Browser* browser,
                                 ToolbarModel* model,
                                 Delegate* delegate,
                                 Mode mode)
                                 : browser_(browser),
                                 model_(model),
                                 delegate_(delegate),
                                 disposition_(CURRENT_TAB),
                                 location_icon_view_(NULL),
                                 location_entry_view_(NULL),
                                 mode_(mode),
                                 show_focus_rect_(false),
                                 animation_offset_(0)
{
    set_id(VIEW_ID_LOCATION_BAR);
    set_focusable(true);

    if(mode_ == NORMAL)
    {
        painter_.reset(new view::HorizontalPainter(kNormalModeBackgroundImages));
    }
}

LocationBarView::~LocationBarView() {}

void LocationBarView::Init()
{
    if(mode_ == POPUP)
    {
        font_ = ui::ResourceBundle::GetSharedInstance().GetFont(
            ui::ResourceBundle::BaseFont);
    }
    else
    {
        // Use a larger version of the system font.
        font_ = ui::ResourceBundle::GetSharedInstance().GetFont(
            ui::ResourceBundle::MediumFont);
    }

    // If this makes the font too big, try to make it smaller so it will fit.
    const int height =
        std::max(GetPreferredSize().height() - (kVerticalEdgeThickness * 2), 0);
    while((font_.GetHeight() > height) && (font_.GetFontSize() > 1))
    {
        font_ = font_.DeriveFont(-1);
    }

    location_icon_view_ = new LocationIconView(this);
    AddChildView(location_icon_view_);
    location_icon_view_->SetVisible(true);
    location_icon_view_->set_drag_controller(this);

    // Initialize the location entry. We do this to avoid a black flash which is
    // visible when the location entry has just been initialized.
    Update(NULL);

    //OnChanged();
}

bool LocationBarView::IsInitialized() const
{
    return location_entry_view_ != NULL;
}

// static
SkColor LocationBarView::GetColor(ColorKind kind)
{
    switch(kind)
    {
    case BACKGROUND:    return gfx::GetSysSkColor(COLOR_WINDOW);
    case TEXT:          return gfx::GetSysSkColor(COLOR_WINDOWTEXT);
    case SELECTED_TEXT: return gfx::GetSysSkColor(COLOR_HIGHLIGHTTEXT);

    case DEEMPHASIZED_TEXT:
        return gfx::AlphaBlend(GetColor(TEXT), GetColor(BACKGROUND), 128);

    default:
        NOTREACHED();
        return GetColor(TEXT);
    }
}

// DropdownBarHostDelegate
void LocationBarView::SetFocusAndSelection(bool select_all)
{
    FocusLocation(select_all);
}

void LocationBarView::SetAnimationOffset(int offset)
{
    animation_offset_ = offset;
}

void LocationBarView::Update(const TabContents* tab_for_state_restoring)
{
    //bool star_enabled = star_view_ && !model_->input_in_progress() &&
    //    edit_bookmarks_enabled_.GetValue();
    //browser_->command_updater()->UpdateCommandEnabled(
    //    IDC_BOOKMARK_PAGE, star_enabled);
    //if(star_view_)
    //{
    //    star_view_->SetVisible(star_enabled);
    //}
    //RefreshContentSettingViews();
    //RefreshPageActionViews();
    //// Don't Update in app launcher mode so that the location entry does not show
    //// a URL or security background.
    //if(mode_ != APP_LAUNCHER)
    //{
    //    location_entry_->Update(tab_for_state_restoring);
    //}
    //OnChanged();
}

void LocationBarView::UpdateContentSettingsIcons()
{
    RefreshContentSettingViews();

    Layout();
    SchedulePaint();
}

void LocationBarView::UpdatePageActions()
{
    //size_t count_before = page_action_views_.size();
    //RefreshPageActionViews();
    //if(page_action_views_.size() != count_before)
    //{
    //    NotificationService::current()->Notify(
    //        chrome::NOTIFICATION_EXTENSION_PAGE_ACTION_COUNT_CHANGED,
    //        Source<LocationBar>(this),
    //        NotificationService::NoDetails());
    //}

    Layout();
    SchedulePaint();
}

void LocationBarView::InvalidatePageActions()
{
    //size_t count_before = page_action_views_.size();
    //DeletePageActionViews();
    //if(page_action_views_.size() != count_before)
    //{
    //    NotificationService::current()->Notify(
    //        chrome::NOTIFICATION_EXTENSION_PAGE_ACTION_COUNT_CHANGED,
    //        Source<LocationBar>(this),
    //        NotificationService::NoDetails());
    //}
}

void LocationBarView::OnFocus()
{
    // Focus the location entry native view.
    //location_entry_->SetFocus();
    GetWidget()->NotifyAccessibilityEvent(
        this, ui::AccessibilityTypes::EVENT_FOCUS, true);
}

void LocationBarView::SetStarToggled(bool on)
{
    //if(star_view_)
    //{
    //    star_view_->SetToggled(on);
    //}
}

void LocationBarView::ShowStarBubble(const Url& url, bool newly_bookmarked)
{
    //gfx::Rect screen_bounds(star_view_->GetImageBounds());
    //// Compensate for some built-in padding in the Star image.
    //screen_bounds.Inset(1, 1, 1, 2);
    //gfx::Point origin(screen_bounds.origin());
    //view::View::ConvertPointToScreen(star_view_, &origin);
    //screen_bounds.set_origin(origin);
    //browser::ShowBookmarkBubbleView(GetWidget(), screen_bounds, star_view_,
    //    browser_->profile(), url, newly_bookmarked);
}

gfx::Point LocationBarView::GetLocationEntryOrigin() const
{
    gfx::Point origin(location_entry_view_->bounds().origin());
    // If the UI layout is RTL, the coordinate system is not transformed and
    // therefore we need to adjust the X coordinate so that bubble appears on the
    // right hand side of the location bar.
    if(base::i18n::IsRTL())
    {
        origin.set_x(width() - origin.x());
    }
    view::View::ConvertPointToScreen(this, &origin);
    return origin;
}

gfx::Size LocationBarView::GetPreferredSize()
{
    return gfx::Size(0, GetThemeProvider()->GetBitmapNamed(mode_==POPUP ?
        IDR_LOCATIONBG_POPUPMODE_CENTER : IDR_LOCATIONBG_C)->height());
}

void LocationBarView::Layout()
{
    //if(!location_entry_.get())
    //{
    //    return;
    //}

    //// TODO(sky): baseline layout.
    //int location_y = kVerticalEdgeThickness;
    //// In some cases (e.g. fullscreen mode) we may have 0 height.  We still want
    //// to position our child views in this case, because other things may be
    //// positioned relative to them (e.g. the "bookmark added" bubble if the user
    //// hits ctrl-d).
    //int location_height = std::max(height() - (kVerticalEdgeThickness * 2), 0);

    //// The edge stroke is 1 px thick.  In popup mode, the edges are drawn by the
    //// omnibox' parent, so there isn't any edge to account for at all.
    //const int kEdgeThickness = (mode_==NORMAL) ? kNormalHorizontalEdgeThickness : 0;
    //// The edit has 1 px of horizontal whitespace inside it before the text.
    //const int kEditInternalSpace = 1;
    //// The space between an item and the edit is the normal item space, minus the
    //// edit's built-in space (so the apparent space will be the same).
    //const int kItemEditPadding =
    //    LocationBarView::kItemPadding - kEditInternalSpace;
    //const int kEdgeEditPadding =
    //    LocationBarView::kEdgeItemPadding - kEditInternalSpace;
    //const int kBubbleVerticalPadding = (mode_ == POPUP) ?
    //    -1 : kBubbleHorizontalPadding;

    //// Start by reserving the padding at the right edge.
    //int entry_width = width() - kEdgeThickness - kEdgeItemPadding;

    //// |location_icon_view_| is visible except when |ev_bubble_view_| or
    //// |selected_keyword_view_| are visible.
    int location_icon_width = 0;
    int ev_bubble_width = 0;
    location_icon_view_->SetVisible(false);
    //ev_bubble_view_->SetVisible(false);
    //const string16 keyword(location_entry_->model()->keyword());
    //const bool is_keyword_hint(location_entry_->model()->is_keyword_hint());
    //const bool show_selected_keyword = !keyword.empty() && !is_keyword_hint;
    //if(show_selected_keyword)
    //{
    //    // Assume the keyword might be hidden.
    //    entry_width -= (kEdgeThickness + kEdgeEditPadding);
    //}
    //else if(model_->GetSecurityLevel() == ToolbarModel::EV_SECURE)
    //{
    //    ev_bubble_view_->SetVisible(true);
    //    ev_bubble_view_->SetLabel(UTF16ToWideHack(model_->GetEVCertName()));
    //    ev_bubble_width = ev_bubble_view_->GetPreferredSize().width();
    //    // We'll adjust this width and take it out of |entry_width| below.
    //}
    //else
    //{
          location_icon_view_->SetVisible(true);
          location_icon_width = location_icon_view_->GetPreferredSize().width();
          //entry_width -= (kEdgeThickness + kEdgeItemPadding + location_icon_width +
          //    kItemEditPadding);
    //}

    //if(star_view_ && star_view_->IsVisible())
    //{
    //    entry_width -= star_view_->GetPreferredSize().width() + kItemPadding;
    //}
    //for(PageActionViews::const_iterator i(page_action_views_.begin());
    //    i!=page_action_views_.end(); ++i)
    //{
    //    if((*i)->IsVisible())
    //    {
    //        entry_width -= ((*i)->GetPreferredSize().width() + kItemPadding);
    //    }
    //}
    //for(ContentSettingViews::const_iterator i(content_setting_views_.begin());
    //    i != content_setting_views_.end(); ++i)
    //{
    //    if((*i)->IsVisible())
    //    {
    //        entry_width -= ((*i)->GetPreferredSize().width() + kItemPadding);
    //    }
    //}
    //// The gap between the edit and whatever is to its right is shortened.
    //entry_width += kEditInternalSpace;

    //// Size the EV bubble.  We do this after taking the star/page actions/content
    //// settings out of |entry_width| so we won't take too much space.
    //if(ev_bubble_width)
    //{
    //    // Try to elide the bubble to be no larger than half the total available
    //    // space, but never elide it any smaller than 150 px.
    //    static const int kMinElidedBubbleWidth = 150;
    //    static const double kMaxBubbleFraction = 0.5;
    //    const int total_padding =
    //        kEdgeThickness + kBubbleHorizontalPadding + kItemEditPadding;
    //    ev_bubble_width = std::min(ev_bubble_width, std::max(kMinElidedBubbleWidth,
    //        static_cast<int>((entry_width - total_padding) * kMaxBubbleFraction)));
    //    entry_width -= (total_padding + ev_bubble_width);
    //}

    //int max_edit_width = entry_width;
    //if(view::Widget::IsPureViews())
    //{
    //    NOTIMPLEMENTED();
    //}
    //else
    //{
    //    RECT formatting_rect;
    //    GetOmniboxViewWin()->GetRect(&formatting_rect);
    //    RECT edit_bounds;
    //    GetOmniboxViewWin()->GetClientRect(&edit_bounds);
    //    max_edit_width = entry_width - formatting_rect.left -
    //        (edit_bounds.right - formatting_rect.right);
    //}

    //if(max_edit_width < 0)
    //{
    //    return;
    //}
    //const int available_width = AvailableWidth(max_edit_width);

    //const bool show_keyword_hint = !keyword.empty() && is_keyword_hint;
    //selected_keyword_view_->SetVisible(show_selected_keyword);
    //keyword_hint_view_->SetVisible(show_keyword_hint);
    //if(show_selected_keyword)
    //{
    //    if(selected_keyword_view_->keyword() != keyword)
    //    {
    //        selected_keyword_view_->SetKeyword(keyword);
    //        Profile* profile = browser_->profile();
    //        const TemplateURL* template_url =
    //            TemplateURLServiceFactory::GetForProfile(profile)->
    //            GetTemplateURLForKeyword(keyword);
    //        if(template_url && template_url->IsExtensionKeyword())
    //        {
    //            const SkBitmap& bitmap = profile->GetExtensionService()->GetOmniboxIcon(
    //                template_url->GetExtensionId());
    //            selected_keyword_view_->SetImage(bitmap);
    //            selected_keyword_view_->set_is_extension_icon(true);
    //        }
    //        else
    //        {
    //            selected_keyword_view_->SetImage(*ResourceBundle::GetSharedInstance().
    //                GetBitmapNamed(IDR_OMNIBOX_SEARCH));
    //            selected_keyword_view_->set_is_extension_icon(false);
    //        }
    //    }
    //}
    //else if(show_keyword_hint)
    //{
    //    if(keyword_hint_view_->keyword() != keyword)
    //    {
    //        keyword_hint_view_->SetKeyword(keyword);
    //    }
    //}

    //// Lay out items to the right of the edit field.
    //int offset = width() - kEdgeThickness - kEdgeItemPadding;
    //if(star_view_ && star_view_->IsVisible())
    //{
    //    int star_width = star_view_->GetPreferredSize().width();
    //    offset -= star_width;
    //    star_view_->SetBounds(offset, location_y, star_width, location_height);
    //    offset -= kItemPadding;
    //}

    //for(PageActionViews::const_iterator i(page_action_views_.begin());
    //    i!=page_action_views_.end(); ++i)
    //{
    //    if((*i)->IsVisible())
    //    {
    //        int page_action_width = (*i)->GetPreferredSize().width();
    //        offset -= page_action_width;
    //        (*i)->SetBounds(offset, location_y, page_action_width, location_height);
    //        offset -= kItemPadding;
    //    }
    //}
    //// We use a reverse_iterator here because we're laying out the views from
    //// right to left but in the vector they're ordered left to right.
    //for(ContentSettingViews::const_reverse_iterator i(content_setting_views_.rbegin());
    //    i!=content_setting_views_.rend(); ++i)
    //{
    //    if((*i)->IsVisible())
    //    {
    //        int content_blocked_width = (*i)->GetPreferredSize().width();
    //        offset -= content_blocked_width;
    //        (*i)->SetBounds(offset, location_y, content_blocked_width,
    //            location_height);
    //        offset -= kItemPadding;
    //    }
    //}

    //// Now lay out items to the left of the edit field.
    //if(location_icon_view_->IsVisible())
    //{
    //    location_icon_view_->SetBounds(kEdgeThickness + kEdgeItemPadding,
    //        location_y, location_icon_width, location_height);
    //    offset = location_icon_view_->bounds().right() + kItemEditPadding;
    //}
    //else if(ev_bubble_view_->IsVisible())
    //{
    //    ev_bubble_view_->SetBounds(kEdgeThickness + kBubbleHorizontalPadding,
    //        location_y + kBubbleVerticalPadding, ev_bubble_width,
    //        ev_bubble_view_->GetPreferredSize().height());
    //    offset = ev_bubble_view_->bounds().right() + kItemEditPadding;
    //}
    //else
    //{
    //    offset = kEdgeThickness +
    //        (show_selected_keyword ? kBubbleHorizontalPadding : kEdgeEditPadding);
    //}

    //// Now lay out the edit field and views that autocollapse to give it more
    //// room.
    //gfx::Rect location_bounds(offset, location_y, entry_width, location_height);
    //if(show_selected_keyword)
    //{
    //    selected_keyword_view_->SetBounds(0, location_y + kBubbleVerticalPadding,
    //        0, selected_keyword_view_->GetPreferredSize().height());
    //    LayoutView(selected_keyword_view_, kItemEditPadding, available_width,
    //        true, &location_bounds);
    //    location_bounds.set_x(selected_keyword_view_->IsVisible() ?
    //        (offset + selected_keyword_view_->width() + kItemEditPadding) :
    //    (kEdgeThickness + kEdgeEditPadding));
    //}
    //else if(show_keyword_hint)
    //{
    //    keyword_hint_view_->SetBounds(0, location_y, 0, location_height);
    //    // Tricky: |entry_width| has already been enlarged by |kEditInternalSpace|.
    //    // But if we add a trailing view, it needs to have that enlargement be to
    //    // its left.  So we undo the enlargement, then include it in the padding for
    //    // the added view.
    //    location_bounds.Inset(0, 0, kEditInternalSpace, 0);
    //    LayoutView(keyword_hint_view_, kItemEditPadding, available_width, false,
    //        &location_bounds);
    //    if(!keyword_hint_view_->IsVisible())
    //    {
    //        // Put back the enlargement that we undid above.
    //        location_bounds.Inset(0, 0, -kEditInternalSpace, 0);
    //    }
    //}

    //// Layout out the suggested text view right aligned to the location
    //// entry. Only show the suggested text if we can fit the text from one
    //// character before the end of the selection to the end of the text and the
    //// suggested text. If we can't it means either the suggested text is too big,
    //// or the user has scrolled.

    //// TODO(sky): We could potentially combine this with the previous step to
    //// force using minimum size if necessary, but currently the chance of showing
    //// keyword hints and suggested text is minimal and we're not confident this
    //// is the right approach for suggested text.
    //if(suggested_text_view_)
    //{
    //    if(view::Widget::IsPureViews())
    //    {
    //        NOTIMPLEMENTED();
    //    }
    //    else
    //    {
    //        // TODO(sky): need to layout when the user changes caret position.
    //        int suggested_text_width =
    //            suggested_text_view_->GetPreferredSize().width();
    //        int vis_text_width = GetOmniboxViewWin()->WidthOfTextAfterCursor();
    //        if(vis_text_width + suggested_text_width > entry_width)
    //        {
    //            // Hide the suggested text if the user has scrolled or we can't fit all
    //            // the suggested text.
    //            suggested_text_view_->SetBounds(0, 0, 0, 0);
    //        }
    //        else
    //        {
    //            int location_needed_width = location_entry_->TextWidth();
    //            location_bounds.set_width(std::min(location_needed_width,
    //                entry_width - suggested_text_width));
    //            // TODO(sky): figure out why this needs the -1.
    //            suggested_text_view_->SetBounds(location_bounds.right() - 1,
    //                location_bounds.y(),
    //                suggested_text_width,
    //                location_bounds.height());
    //        }
    //    }
    //}

    //location_entry_view_->SetBoundsRect(location_bounds);
}

void LocationBarView::OnPaint(gfx::Canvas* canvas)
{
    View::OnPaint(canvas);

    if(painter_.get())
    {
        painter_->Paint(width(), height(), canvas);
    }
    else if(mode_ == POPUP)
    {
        canvas->TileImageInt(*GetThemeProvider()->GetBitmapNamed(
            IDR_LOCATIONBG_POPUPMODE_CENTER), 0, 0, 0, 0, width(), height());
    }
    // When used in the app launcher, don't draw a border, the LocationBarView has
    // its own view::Border.

    // Draw the background color so that the graphical elements at the edges
    // appear over the correct color.  (The edit draws its own background, so this
    // isn't important for that.)
    // TODO(pkasting): We need images that are transparent in the middle, so we
    // can draw the border images over the background color instead of the
    // reverse; this antialiases better (see comments in
    // AutocompletePopupContentsView::OnPaint()).
    gfx::Rect bounds(GetContentsBounds());
    bounds.Inset(0, kVerticalEdgeThickness);
    SkColor color(GetColor(BACKGROUND));
    if(mode_ == NORMAL)
    {
        SkPaint paint;
        paint.setColor(color);
        paint.setStyle(SkPaint::kFill_Style);
        paint.setAntiAlias(true);
        // The round corners of the omnibox match the round corners of the dropdown
        // below, and all our other bubbles.
        const SkScalar radius(SkIntToScalar(
            BubbleBorder::GetCornerRadius()));
        bounds.Inset(kNormalHorizontalEdgeThickness, 0);
        canvas->AsCanvasSkia()->drawRoundRect(gfx::RectToSkRect(bounds), radius,
            radius, paint);
    }
    else
    {
        canvas->FillRectInt(color, bounds.x(), bounds.y(), bounds.width(),
            bounds.height());
    }

    if(show_focus_rect_ && HasFocus())
    {
        gfx::Rect r = location_entry_view_->bounds();
        canvas->DrawFocusRect(r.x() - 1, r.y() - 1, r.width() + 2, r.height() + 2);
    }
}

void LocationBarView::SetShowFocusRect(bool show)
{
    show_focus_rect_ = show;
    SchedulePaint();
}

void LocationBarView::SelectAll()
{
    //location_entry_->SelectAll(true);
}

bool LocationBarView::OnMousePressed(const view::MouseEvent& event)
{
    UINT msg;
    if(event.IsLeftMouseButton())
    {
        msg = (event.flags() & ui::EF_IS_DOUBLE_CLICK) ?
            WM_LBUTTONDBLCLK : WM_LBUTTONDOWN;
    }
    else if(event.IsMiddleMouseButton())
    {
        msg = (event.flags() & ui::EF_IS_DOUBLE_CLICK) ?
            WM_MBUTTONDBLCLK : WM_MBUTTONDOWN;
    }
    else if(event.IsRightMouseButton())
    {
        msg = (event.flags() & ui::EF_IS_DOUBLE_CLICK) ?
            WM_RBUTTONDBLCLK : WM_RBUTTONDOWN;
    }
    else
    {
        NOTREACHED();
        return false;
    }
    OnMouseEvent(event, msg);
    return true;
}

bool LocationBarView::OnMouseDragged(const view::MouseEvent& event)
{
    OnMouseEvent(event, WM_MOUSEMOVE);
    return true;
}

void LocationBarView::OnMouseReleased(const view::MouseEvent& event)
{
    UINT msg;
    if(event.IsLeftMouseButton())
    {
        msg = WM_LBUTTONUP;
    }
    else if(event.IsMiddleMouseButton())
    {
        msg = WM_MBUTTONUP;
    }
    else if(event.IsRightMouseButton())
    {
        msg = WM_RBUTTONUP;
    }
    else
    {
        NOTREACHED();
        return;
    }
    OnMouseEvent(event, msg);
}

void LocationBarView::OnMouseCaptureLost()
{
    if(view::Widget::IsPureViews())
    {
        NOTIMPLEMENTED();
    }
    else
    {
        //GetOmniboxViewWin()->HandleExternalMsg(WM_CAPTURECHANGED, 0, CPoint());
    }
}

int LocationBarView::AvailableWidth(int location_bar_width)
{
    return location_bar_width/* - location_entry_->TextWidth()*/;
}

void LocationBarView::LayoutView(view::View* view,
                                 int padding,
                                 int available_width,
                                 bool leading,
                                 gfx::Rect* bounds)
{
    DCHECK(view && bounds);
    gfx::Size view_size = view->GetPreferredSize();
    if((view_size.width() + padding) > available_width)
    {
        view_size = view->GetMinimumSize();
    }
    int desired_width = view_size.width() + padding;
    view->SetVisible(desired_width < bounds->width());
    if(view->IsVisible())
    {
        view->SetBounds(
            leading ? bounds->x() : (bounds->right() - view_size.width()),
            view->y(), view_size.width(), view->height());
        bounds->set_width(bounds->width() - desired_width);
    }
}

void LocationBarView::RefreshContentSettingViews()
{
    //for(ContentSettingViews::const_iterator i(content_setting_views_.begin());
    //    i!=content_setting_views_.end(); ++i)
    //{
    //    (*i)->UpdateFromTabContents(model_->input_in_progress() ? NULL :
    //        GetTabContentsFromDelegate(delegate_));
    //}
}

void LocationBarView::OnMouseEvent(const view::MouseEvent& event, UINT msg)
{
    UINT flags = event.GetWindowsFlags();
    gfx::Point screen_point(event.location());
    ConvertPointToScreen(this, &screen_point);
    if(view::Widget::IsPureViews())
    {
        NOTIMPLEMENTED();
    }
    else
    {
        //GetOmniboxViewWin()->HandleExternalMsg(msg, flags, screen_point.ToPOINT());
    }
}

std::string LocationBarView::GetClassName() const
{
    return kViewClassName;
}

bool LocationBarView::SkipDefaultKeyEventProcessing(
    const view::KeyEvent& event)
{
    //bool views_omnibox = view::Widget::IsPureViews();
    //if(view::FocusManager::IsTabTraversalKeyEvent(event))
    //{
    //    if(HasValidSuggestText())
    //    {
    //        // Return true so that the edit sees the tab and commits the suggestion.
    //        return true;
    //    }
    //    if(keyword_hint_view_->IsVisible() && !event.IsShiftDown())
    //    {
    //        // Return true so the edit gets the tab event and enters keyword mode.
    //        return true;
    //    }

    //    // If the caret is not at the end, then tab moves the caret to the end.
    //    if(!views_omnibox && !GetOmniboxViewWin()->IsCaretAtEnd())
    //    {
    //        return true;
    //    }

    //    // Tab while showing instant commits instant immediately.
    //    // Return true so that focus traversal isn't attempted. The edit ends
    //    // up doing nothing in this case.
    //    if(location_entry_->model()->AcceptCurrentInstantPreview())
    //    {
    //        return true;
    //    }
    //}

    //if(!views_omnibox)
    //{
    //    return GetOmniboxViewWin()->SkipDefaultKeyEventProcessing(event);
    //}
    //NOTIMPLEMENTED();
    return false;
}

void LocationBarView::GetAccessibleState(ui::AccessibleViewState* state)
{
    state->role = ui::AccessibilityTypes::ROLE_LOCATION_BAR;
    state->name = ui::GetStringUTF16(IDS_ACCNAME_LOCATION);
    //state->value = location_entry_->GetText();

    //string16::size_type entry_start;
    //string16::size_type entry_end;
    //location_entry_->GetSelectionBounds(&entry_start, &entry_end);
    //state->selection_start = entry_start;
    //state->selection_end = entry_end;
}

void LocationBarView::WriteDragDataForView(view::View* sender,
                                           const gfx::Point& press_pt,
                                           ui::OSExchangeData* data)
{
    DCHECK_NE(GetDragOperationsForView(sender, press_pt),
        ui::DragDropTypes::DRAG_NONE);

    TabContentsWrapper* tab_contents = delegate_->GetTabContentsWrapper();
    DCHECK(tab_contents);
    //drag_utils::SetURLAndDragImage(
    //    tab_contents->tab_contents()->GetURL(),
    //    tab_contents->tab_contents()->GetTitle(),
    //    tab_contents->favicon_tab_helper()->GetFavicon(),
    //    data);
}

int LocationBarView::GetDragOperationsForView(view::View* sender,
                                              const gfx::Point& p)
{
    return ui::DragDropTypes::DRAG_NONE;
    //DCHECK((sender == location_icon_view_) || (sender == ev_bubble_view_));
    //TabContents* tab_contents = GetTabContentsFromDelegate(delegate_);
    //return (tab_contents && tab_contents->GetURL().is_valid() &&
    //    !location_entry()->IsEditingOrEmpty()) ?
    //    (ui::DragDropTypes::DRAG_COPY | ui::DragDropTypes::DRAG_LINK) :
    //    ui::DragDropTypes::DRAG_NONE;
}

bool LocationBarView::CanStartDragForView(View* sender,
                                          const gfx::Point& press_pt,
                                          const gfx::Point& p)
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// LocationBarView, LocationBar implementation:

void LocationBarView::SetSuggestedText(const string16& text)
{
    //location_entry_->model()->SetSuggestedText(text, behavior);
}

string16 LocationBarView::GetInputString() const
{
    return location_input_;
}

WindowOpenDisposition LocationBarView::GetWindowOpenDisposition() const
{
    return disposition_;
}

void LocationBarView::AcceptInput()
{
    //location_entry_->model()->AcceptInput(CURRENT_TAB, false);
}

void LocationBarView::FocusLocation(bool select_all)
{
    //location_entry_->SetFocus();
    //if(select_all)
    //{
    //    location_entry_->SelectAll(true);
    //}
}

void LocationBarView::FocusSearch()
{
    //location_entry_->SetFocus();
    //location_entry_->SetForcedQuery();
}

void LocationBarView::SaveStateToContents(TabContents* contents)
{
    //location_entry_->SaveStateToTab(contents);
}

void LocationBarView::Revert()
{
    //location_entry_->RevertAll();
}

const OmniboxView* LocationBarView::location_entry() const
{
    return NULL;
    //return location_entry_.get();
}

OmniboxView* LocationBarView::location_entry()
{
    return NULL;
    //return location_entry_.get();
}

bool LocationBarView::HasValidSuggestText() const
{
    return false;
    //return suggested_text_view_ && !suggested_text_view_->size().IsEmpty() &&
    //    !suggested_text_view_->GetText().empty();
}