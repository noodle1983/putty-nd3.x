
#include "base_tab.h"

#include <limits>

#include "base/utf_string_conversions.h"

#include "ui_gfx/canvas_skia.h"
#include "ui_gfx/favicon_size.h"
#include "ui_gfx/font.h"

#include "ui_base/accessibility/accessible_view_state.h"
#include "ui_base/animation/animation_container.h"
#include "ui_base/animation/linear_animation.h"
#include "ui_base/animation/throb_animation.h"
#include "ui_base/l10n/l10n_util.h"
#include "ui_base/resource/app_res_ids.h"
#include "ui_base/resource/resource_bundle.h"
#include "ui_base/theme_provider.h"

#include "../wanui_res/resource.h"

#include "browser.h"
#include "tab_contents_wrapper.h"
#include "tab_controller.h"
#include "view_ids.h"

// How long the pulse throb takes.
static const int kPulseDurationMs = 200;

// How long the hover state takes.
static const int kHoverDurationMs = 400;

namespace
{

    ////////////////////////////////////////////////////////////////////////////////
    // TabCloseButton
    //
    //  This is a Button subclass that causes middle clicks to be forwarded to the
    //  parent View by explicitly not handling them in OnMousePressed.
    class TabCloseButton : public view::ImageButton
    {
    public:
        explicit TabCloseButton(view::ButtonListener* listener)
            : view::ImageButton(listener) {}
        virtual ~TabCloseButton() {}

        virtual bool OnMousePressed(const view::MouseEvent& event)
        {
            bool handled = ImageButton::OnMousePressed(event);
            // Explicitly mark midle-mouse clicks as non-handled to ensure the tab
            // sees them.
            return event.IsOnlyMiddleMouseButton() ? false : handled;
        }

        // We need to let the parent know about mouse state so that it
        // can highlight itself appropriately. Note that Exit events
        // fire before Enter events, so this works.
        virtual void OnMouseEntered(const view::MouseEvent& event)
        {
            CustomButton::OnMouseEntered(event);
            parent()->OnMouseEntered(event);
        }

        virtual void OnMouseExited(const view::MouseEvent& event)
        {
            CustomButton::OnMouseExited(event);
            parent()->OnMouseExited(event);
        }

    private:
        DISALLOW_COPY_AND_ASSIGN(TabCloseButton);
    };

    // Draws the icon image at the center of |bounds|.
    void DrawIconCenter(gfx::Canvas* canvas,
        const SkBitmap& image,
        int image_offset,
        int icon_width,
        int icon_height,
        const gfx::Rect& bounds,
        bool filter)
    {
        // Center the image within bounds.
        int dst_x = bounds.x() - (icon_width - bounds.width()) / 2;
        int dst_y = bounds.y() - (icon_height - bounds.height()) / 2;
        // NOTE: the clipping is a work around for 69528, it shouldn't be necessary.
        canvas->Save();
        canvas->ClipRectInt(dst_x, dst_y, icon_width, icon_height);
        canvas->DrawBitmapInt(image,
            image_offset, 0, icon_width, icon_height,
            dst_x, dst_y, icon_width, icon_height,
            filter);
        canvas->Restore();
    }

}

// static
gfx::Font* BaseTab::font_ = NULL;
// static
int BaseTab::font_height_ = 0;

////////////////////////////////////////////////////////////////////////////////
// FaviconCrashAnimation
//
//  A custom animation subclass to manage the favicon crash animation.
class BaseTab::FaviconCrashAnimation : public ui::LinearAnimation,
    public ui::AnimationDelegate
{
public:
    explicit FaviconCrashAnimation(BaseTab* target)
        : ui::LinearAnimation(1000, 25, this),
        target_(target) {}
    virtual ~FaviconCrashAnimation() {}

    // ui::Animation overrides:
    virtual void AnimateToState(double state)
    {
        const double kHidingOffset = 27;

        if(state < .5)
        {
            target_->SetFaviconHidingOffset(
                static_cast<int>(floor(kHidingOffset * 2.0 * state)));
        }
        else
        {
            target_->DisplayCrashedFavicon();
            target_->SetFaviconHidingOffset(
                static_cast<int>(
                floor(kHidingOffset - ((state - .5) * 2.0 * kHidingOffset))));
        }
    }

    // ui::AnimationDelegate overrides:
    virtual void AnimationCanceled(const ui::Animation* animation)
    {
        target_->SetFaviconHidingOffset(0);
    }

private:
    BaseTab* target_;

    DISALLOW_COPY_AND_ASSIGN(FaviconCrashAnimation);
};

BaseTab::BaseTab(TabController* controller)
: controller_(controller),
closing_(false),
dragging_(false),
favicon_hiding_offset_(0),
loading_animation_frame_(0),
should_display_crashed_favicon_(false),
throbber_disabled_(false),
theme_provider_(NULL)
{
    BaseTab::InitResources();

    set_id(VIEW_ID_TAB);

    // Add the Close Button.
    close_button_ = new TabCloseButton(this);
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    close_button_->SetImage(view::CustomButton::BS_NORMAL,
        rb.GetBitmapNamed(IDR_TAB_CLOSE));
    close_button_->SetImage(view::CustomButton::BS_HOT,
        rb.GetBitmapNamed(IDR_TAB_CLOSE_H));
    close_button_->SetImage(view::CustomButton::BS_PUSHED,
        rb.GetBitmapNamed(IDR_TAB_CLOSE_P));
    close_button_->SetTooltipText(
        UTF16ToWide(ui::GetStringUTF16(IDS_TOOLTIP_CLOSE_TAB)));
    close_button_->SetAccessibleName(
        ui::GetStringUTF16(IDS_ACCNAME_CLOSE));
    // Disable animation so that the red danger sign shows up immediately
    // to help avoid mis-clicks.
    close_button_->SetAnimationDuration(0);
    AddChildView(close_button_);

    set_context_menu_controller(this);
}

BaseTab::~BaseTab() {}

void BaseTab::SetData(const TabRendererData& data)
{
    if(data_.Equals(data))
    {
        return;
    }

    TabRendererData old(data_);
    data_ = data;
	ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
	data_.favicon = *rb.GetBitmapNamed(IDR_PRODUCT_LOGO_16);

    if(data_.IsCrashed())
    {
        if(!should_display_crashed_favicon_ && !IsPerformingCrashAnimation())
        {
            StartCrashAnimation();
        }
    }
    else
    {
        if(IsPerformingCrashAnimation())
        {
            StopCrashAnimation();
        }
        ResetCrashedFavicon();
    }

    DataChanged(old);

    Layout();
    SchedulePaint();
}

void BaseTab::UpdateLoadingAnimation(TabRendererData::NetworkState state)
{
    if(state==data_.network_state && state==TabRendererData::NETWORK_STATE_NONE)
    {
        // If the network state is none and hasn't changed, do nothing. Otherwise we
        // need to advance the animation frame.
        return;
    }

    TabRendererData::NetworkState old_state = data_.network_state;
    data_.network_state = state;
    AdvanceLoadingAnimation(old_state, state);
}

void BaseTab::StartPulse()
{
    if(!pulse_animation_.get())
    {
        pulse_animation_.reset(new ui::ThrobAnimation(this));
        pulse_animation_->SetSlideDuration(kPulseDurationMs);
        if(animation_container_.get())
        {
            pulse_animation_->SetContainer(animation_container_.get());
        }
    }
    pulse_animation_->Reset();
    pulse_animation_->StartThrobbing(std::numeric_limits<int>::max());
}

void BaseTab::StopPulse()
{
    if(!pulse_animation_.get())
    {
        return;
    }

    pulse_animation_->Stop(); // Do stop so we get notified.
    pulse_animation_.reset(NULL);
}

void BaseTab::set_animation_container(ui::AnimationContainer* container)
{
    animation_container_ = container;
}

bool BaseTab::IsCloseable() const
{
    return controller() ? controller()->IsTabCloseable(this) : true;
}

bool BaseTab::IsActive() const
{
    return controller() ? controller()->IsActiveTab(this) : true;
}

bool BaseTab::IsSelected() const
{
    return controller() ? controller()->IsTabSelected(this) : true;
}

ui::ThemeProvider* BaseTab::GetThemeProvider() const
{
    ui::ThemeProvider* tp = View::GetThemeProvider();
    return tp ? tp : theme_provider_;
}

bool BaseTab::OnMousePressed(const view::MouseEvent& event)
{
    if(!controller())
    {
        return false;
    }

    if(event.IsOnlyLeftMouseButton())
    {
        if(event.IsShiftDown() && event.IsControlDown())
        {
            controller()->AddSelectionFromAnchorTo(this);
        }
        else if(event.IsShiftDown())
        {
            controller()->ExtendSelectionTo(this);
        }
        else if(event.IsControlDown())
        {
            controller()->ToggleSelected(this);
            if(!IsSelected())
            {
                // Don't allow dragging non-selected tabs.
                return false;
            }
        }
        else if(!IsSelected())
        {
            controller()->SelectTab(this);
        }
        else if(IsActive())
        {
            controller()->ClickActiveTab(this);
        }
        controller()->MaybeStartDrag(this, event);
    }
    return true;
}

bool BaseTab::OnMouseDragged(const view::MouseEvent& event)
{
    if(controller())
    {
        controller()->ContinueDrag(event);
    }
    return true;
}

void BaseTab::OnMouseReleased(const view::MouseEvent& event)
{
    if(!controller())
    {
        return;
    }

    // Notify the drag helper that we're done with any potential drag operations.
    // Clean up the drag helper, which is re-created on the next mouse press.
    // In some cases, ending the drag will schedule the tab for destruction; if
    // so, bail immediately, since our members are already dead and we shouldn't
    // do anything else except drop the tab where it is.
    if(controller()->EndDrag(false))
    {
        return;
    }

    // Close tab on middle click, but only if the button is released over the tab
    // (normal windows behavior is to discard presses of a UI element where the
    // releases happen off the element).
    if(event.IsMiddleMouseButton())
    {
        if(HitTest(event.location()))
        {
            controller()->CloseTab(this);
        }
        else if(closing_)
        {
            // We're animating closed and a middle mouse button was pushed on us but
            // we don't contain the mouse anymore. We assume the user is clicking
            // quicker than the animation and we should close the tab that falls under
            // the mouse.
            BaseTab* closest_tab = controller()->GetTabAt(this, event.location());
            if(closest_tab)
            {
                controller()->CloseTab(closest_tab);
            }
        }
    }
    else if(event.IsOnlyLeftMouseButton() && !event.IsShiftDown() &&
        !event.IsControlDown())
    {
        // If the tab was already selected mouse pressed doesn't change the
        // selection. Reset it now to handle the case where multiple tabs were
        // selected.
        controller()->SelectTab(this);
    }
}

void BaseTab::OnMouseCaptureLost()
{
    if(controller())
    {
        controller()->EndDrag(true);
    }
}

void BaseTab::OnMouseEntered(const view::MouseEvent& event)
{
    if(!hover_animation_.get())
    {
        hover_animation_.reset(new ui::SlideAnimation(this));
        hover_animation_->SetContainer(animation_container_.get());
        hover_animation_->SetSlideDuration(kHoverDurationMs);
    }
    hover_animation_->SetTweenType(ui::Tween::EASE_OUT);
    hover_animation_->Show();
}

void BaseTab::OnMouseExited(const view::MouseEvent& event)
{
    hover_animation_->SetTweenType(ui::Tween::EASE_IN);
    hover_animation_->Hide();
}

bool BaseTab::GetTooltipText(const gfx::Point& p, std::wstring* tooltip)
{
    if(data_.title.empty())
    {
        return false;
    }

    // Only show the tooltip if the title is truncated.
    if(font_->GetStringWidth(data_.title) > GetTitleBounds().width())
    {
        *tooltip = UTF16ToWide(data_.title);
        return true;
    }
    return false;
}

void BaseTab::GetAccessibleState(ui::AccessibleViewState* state)
{
    state->role = ui::AccessibilityTypes::ROLE_PAGETAB;
}

void BaseTab::AdvanceLoadingAnimation(TabRendererData::NetworkState old_state,
                                      TabRendererData::NetworkState state)
{
    static bool initialized = false;
    static int loading_animation_frame_count = 0;
    static int waiting_animation_frame_count = 0;
    static int waiting_to_loading_frame_count_ratio = 0;
    if(!initialized)
    {
        initialized = true;
        ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
        SkBitmap loading_animation(*rb.GetBitmapNamed(IDR_THROBBER));
        loading_animation_frame_count =
            loading_animation.width() / loading_animation.height();
        SkBitmap waiting_animation(*rb.GetBitmapNamed(IDR_THROBBER_WAITING));
        waiting_animation_frame_count =
            waiting_animation.width() / waiting_animation.height();
        waiting_to_loading_frame_count_ratio =
            waiting_animation_frame_count / loading_animation_frame_count;
    }

    // The waiting animation is the reverse of the loading animation, but at a
    // different rate - the following reverses and scales the animation_frame_
    // so that the frame is at an equivalent position when going from one
    // animation to the other.
    if(state != old_state)
    {
        loading_animation_frame_ = loading_animation_frame_count -
            (loading_animation_frame_ / waiting_to_loading_frame_count_ratio);
    }

    if(state != TabRendererData::NETWORK_STATE_NONE)
    {
        loading_animation_frame_ = (loading_animation_frame_ + 1) %
            ((state == TabRendererData::NETWORK_STATE_WAITING) ?
            waiting_animation_frame_count : loading_animation_frame_count);
    }
    else
    {
        loading_animation_frame_ = 0;
    }
    ScheduleIconPaint();
}

void BaseTab::PaintIcon(gfx::Canvas* canvas)
{
    gfx::Rect bounds = GetIconBounds();
    if(bounds.IsEmpty())
    {
        return;
    }

    bounds.set_x(GetMirroredXForRect(bounds));

    if(data().network_state == TabRendererData::NETWORK_STATE_WAITING
		|| data().network_state == TabRendererData::NETWORK_STATE_LOADING)
    {
        ui::ThemeProvider* tp = GetThemeProvider();
        SkBitmap frames(*tp->GetBitmapNamed(
            (data().network_state == TabRendererData::NETWORK_STATE_WAITING) ?
            IDR_THROBBER_WAITING : IDR_THROBBER));

        int icon_size = frames.height();
        int image_offset = loading_animation_frame_ * icon_size;
        DrawIconCenter(canvas, frames, image_offset,
            icon_size, icon_size, bounds, false);
    }
    else if (data().network_state == TabRendererData::NETWORK_STATE_DISCONNECTED)
	{
		ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
        SkBitmap crashed_favicon(*rb.GetBitmapNamed(IDR_SAD_FAVICON));
        bounds.set_y(bounds.y() + favicon_hiding_offset_);
        DrawIconCenter(canvas, crashed_favicon, 0,
            crashed_favicon.width(),
            crashed_favicon.height(), bounds, true);
	}else
    {
        canvas->Save();
        canvas->ClipRectInt(0, 0, width(), height());
        if(should_display_crashed_favicon_)
        {
            ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
            SkBitmap crashed_favicon(*rb.GetBitmapNamed(IDR_SAD_FAVICON));
            bounds.set_y(bounds.y() + favicon_hiding_offset_);
            DrawIconCenter(canvas, crashed_favicon, 0,
                crashed_favicon.width(),
                crashed_favicon.height(), bounds, true);
        }
        else
        {
            if(!data().favicon.isNull())
            {
                // TODO(pkasting): Use code in tab_icon_view.cc:PaintIcon() (or switch
                // to using that class to render the favicon).
                DrawIconCenter(canvas, data().favicon, 0,
                    data().favicon.width(),
                    data().favicon.height(),
                    bounds, true);
            }
        }
        canvas->Restore();
    }
}

void BaseTab::PaintTitle(gfx::Canvas* canvas, SkColor title_color)
{
    // Paint the Title.
    const gfx::Rect& title_bounds = GetTitleBounds();
    string16 title = data().title;
	/*if (data().network_state == TabRendererData::NETWORK_STATE_DISCONNECTED)
	{
		title_color = SK_ColorGRAY;
	}*/

    if(title.empty())
    {
        title = data().loading ?
            ui::GetStringUTF16(IDS_TAB_LOADING_TITLE) :
        TabContentsWrapper::GetDefaultTitle();
    }
    else
    {
        Browser::FormatTitleForDisplay(&title);
    }

    canvas->AsCanvasSkia()->DrawFadeTruncatingString(title,
        gfx::CanvasSkia::TruncateFadeTail, 0, *font_, title_color, title_bounds);
}

void BaseTab::AnimationProgressed(const ui::Animation* animation)
{
    SchedulePaint();
}

void BaseTab::AnimationCanceled(const ui::Animation* animation)
{
    SchedulePaint();
}

void BaseTab::AnimationEnded(const ui::Animation* animation)
{
    SchedulePaint();
}

void BaseTab::ButtonPressed(view::Button* sender, const view::Event& event)
{
    DCHECK(sender == close_button_);
    controller()->CloseTab(this);
}

void BaseTab::ShowContextMenuForView(view::View* source,
                                     const gfx::Point& p,
                                     bool is_mouse_gesture)
{
    if(controller() && !closing())
    {
        controller()->ShowContextMenuForTab(this, p);
    }
}

int BaseTab::loading_animation_frame() const
{
    return loading_animation_frame_;
}

bool BaseTab::should_display_crashed_favicon() const
{
    return should_display_crashed_favicon_;
}

int BaseTab::favicon_hiding_offset() const
{
    return favicon_hiding_offset_;
}

void BaseTab::SetFaviconHidingOffset(int offset)
{
    favicon_hiding_offset_ = offset;
    ScheduleIconPaint();
}

void BaseTab::DisplayCrashedFavicon()
{
    should_display_crashed_favicon_ = true;
}

void BaseTab::ResetCrashedFavicon()
{
    should_display_crashed_favicon_ = false;
}

void BaseTab::StartCrashAnimation()
{
    if(!crash_animation_.get())
    {
        crash_animation_.reset(new FaviconCrashAnimation(this));
    }
    crash_animation_->Stop();
    crash_animation_->Start();
}

void BaseTab::StopCrashAnimation()
{
    if(!crash_animation_.get())
    {
        return;
    }
    crash_animation_->Stop();
}

bool BaseTab::IsPerformingCrashAnimation() const
{
    return crash_animation_.get() && crash_animation_->is_animating();
}

void BaseTab::ScheduleIconPaint()
{
    gfx::Rect bounds = GetIconBounds();
    if(bounds.IsEmpty())
    {
        return;
    }

    // Extends the area to the bottom when sad_favicon is
    // animating.
    if(IsPerformingCrashAnimation())
    {
        bounds.set_height(height() - bounds.y());
    }
    bounds.set_x(GetMirroredXForRect(bounds));
    SchedulePaintInRect(bounds);
}

// static
void BaseTab::InitResources()
{
    static bool initialized = false;
    if(!initialized)
    {
        initialized = true;
        font_ = new gfx::Font(
            ui::ResourceBundle::GetSharedInstance().GetFont(
            ui::ResourceBundle::BaseFont));
        font_height_ = font_->GetHeight();
    }
}