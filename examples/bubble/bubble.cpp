
#include "bubble.h"

#include "ui_gfx/color_utils.h"

#include "ui_base/animation/slide_animation.h"

#include "view/layout/fill_layout.h"

#include "border_content.h"
#include "border_widget_win.h"

// How long the fade should last for.
static const int kHideFadeDurationMS = 200;

// Background color of the bubble.
const SkColor Bubble::kBackgroundColor = gfx::GetSysSkColor(COLOR_WINDOW);

// BubbleDelegate ---------------------------------------------------------

std::wstring BubbleDelegate::accessible_name()
{
    return L"";
}

// Bubble -----------------------------------------------------------------

// static
Bubble* Bubble::Show(view::Widget* parent,
                     const gfx::Rect& position_relative_to,
                     BubbleBorder::ArrowLocation arrow_location,
                     view::View* content,
                     BubbleDelegate* delegate)
{
    Bubble* bubble = new Bubble;
    bubble->InitBubble(parent, position_relative_to, arrow_location,
        content, delegate);

    // Register the Escape accelerator for closing.
    bubble->RegisterEscapeAccelerator();

    return bubble;
}

void Bubble::Close()
{
    if(show_status_ != kOpen)
    {
        return;
    }

    show_status_ = kClosing;

    if(fade_away_on_close_)
    {
        FadeOut();
    }
    else
    {
        DoClose(false);
    }
}

void Bubble::AnimationEnded(const ui::Animation* animation)
{
    if(static_cast<int>(animation_->GetCurrentValue()) == 0)
    {
        // When fading out we just need to close the bubble at the end
        DoClose(false);
    }
    else
    {
        // When fading in we need to remove the layered window style flag, since
        // that style prevents some bubble content from working properly.
        SetWindowLong(GWL_EXSTYLE, GetWindowLong(GWL_EXSTYLE) & ~WS_EX_LAYERED);
    }
}

void Bubble::AnimationProgressed(const ui::Animation* animation)
{
    // Set the opacity for the main content window.
    unsigned char opacity = static_cast<unsigned char>(
        animation_->GetCurrentValue() * 255);
    SetLayeredWindowAttributes(GetNativeView(), 0,
        static_cast<byte>(opacity), LWA_ALPHA);
    content_->SchedulePaint();

    // Also fade in/out the bubble border window.
    border_->SetOpacity(opacity);
    border_->border_content()->SchedulePaint();
}

Bubble::Bubble()
: view::NativeWidgetWin(new view::Widget),
border_(NULL),
delegate_(NULL),
show_status_(kOpen),
fade_away_on_close_(false),
arrow_location_(BubbleBorder::NONE),
content_(NULL),
accelerator_registered_(false) {}

Bubble::~Bubble() {}

void Bubble::InitBubble(view::Widget* parent,
                        const gfx::Rect& position_relative_to,
                        BubbleBorder::ArrowLocation arrow_location,
                        view::View* content,
                        BubbleDelegate* delegate)
{
    delegate_ = delegate;
    position_relative_to_ = position_relative_to;
    arrow_location_ = arrow_location;
    content_ = content;
    const bool fade_in = delegate_ && delegate_->FadeInOnShow();

    // Create the main window.
    view::Widget* parent_window = parent->GetTopLevelWidget();
    if(parent_window)
    {
        parent_window->DisableInactiveRendering();
    }
    set_window_style(WS_POPUP | WS_CLIPCHILDREN);
    int extended_style = WS_EX_TOOLWINDOW;
    // During FadeIn we need to turn on the layered window style to deal with
    // transparency. This flag needs to be reset after fading in is complete.
    if(fade_in)
    {
        extended_style |= WS_EX_LAYERED;
    }
    set_window_ex_style(extended_style);

    DCHECK(!border_);
    border_ = new BorderWidgetWin();

    border_->InitBorderWidgetWin(CreateBorderContent(), parent->GetNativeView());
    border_->border_content()->SetBackgroundColor(kBackgroundColor);

    // We make the BorderWidgetWin the owner of the Bubble HWND, so that the
    // latter is displayed on top of the former.
    view::Widget::InitParams params(view::Widget::InitParams::TYPE_POPUP);
    params.parent = border_->GetNativeView();
    params.native_widget = this;
    GetWidget()->Init(params);

    if(fade_in)
    {
        border_->SetOpacity(0);
        GetWidget()->SetOpacity(0);
    }
    if(delegate_)
    {
        SetWindowText(GetNativeView(), delegate_->accessible_name().c_str());
    }

    // Create a View to hold the content of the main window.
    view::View* content_view = new view::View;
    // We add |content_view| to ourselves before the AddChildView() call below so
    // that when |content| gets added, it will already have a widget, and thus
    // any NativeButtons it creates in ViewHierarchyChanged() will be functional
    // (e.g. calling SetChecked() on checkboxes is safe).
    GetWidget()->SetContentsView(content_view);
    // Adding |content| as a child has to be done before we call
    // content->GetPreferredSize() below, since some supplied views don't
    // actually initialize themselves until they're added to a hierarchy.
    content_view->AddChildView(content);

    // Calculate and set the bounds for all windows and views.
    gfx::Rect window_bounds;

    // Initialize and position the border window.
    window_bounds = border_->SizeAndGetBounds(position_relative_to,
        arrow_location, content->GetPreferredSize());

    // Make |content| take up the entire content view.
    content_view->SetLayoutManager(new view::FillLayout);

    // Paint the background color behind the content.
    content_view->set_background(
        view::Background::CreateSolidBackground(kBackgroundColor));

    GetWidget()->SetBounds(window_bounds);

    // Show the window.
    border_->ShowWindow(SW_SHOW);
    ShowWindow(SW_SHOW);
    if(fade_in)
    {
        FadeIn();
    }
}

void Bubble::RegisterEscapeAccelerator()
{
    GetWidget()->GetFocusManager()->RegisterAccelerator(
        view::Accelerator(ui::VKEY_ESCAPE, false, false, false), this);
    accelerator_registered_ = true;
}

void Bubble::UnregisterEscapeAccelerator()
{
    DCHECK(accelerator_registered_);
    GetWidget()->GetFocusManager()->UnregisterAccelerator(
        view::Accelerator(ui::VKEY_ESCAPE, false, false, false), this);
    accelerator_registered_ = false;
}

BorderContent* Bubble::CreateBorderContent()
{
    return new BorderContent();
}

void Bubble::SizeToContent()
{
    gfx::Rect window_bounds;

    // Initialize and position the border window.
    window_bounds = border_->SizeAndGetBounds(position_relative_to_,
        arrow_location_, content_->GetPreferredSize());
    GetWidget()->SetBounds(window_bounds);
}

void Bubble::OnActivate(UINT action, BOOL minimized, HWND window)
{
    // The popup should close when it is deactivated.
    if(action == WA_INACTIVE)
    {
        GetWidget()->Close();
    }
    else if(action == WA_ACTIVE)
    {
        DCHECK(GetWidget()->GetRootView()->has_children());
        GetWidget()->GetRootView()->child_at(0)->RequestFocus();
    }
}

void Bubble::DoClose(bool closed_by_escape)
{
    if(show_status_ == kClosed)
    {
        return;
    }

    if(accelerator_registered_)
    {
        UnregisterEscapeAccelerator();
    }
    if(delegate_)
    {
        delegate_->BubbleClosing(this, closed_by_escape);
    }
    show_status_ = kClosed;
    border_->Close();
    NativeWidgetWin::Close();
}

void Bubble::FadeIn()
{
    Fade(true); // |fade_in|.
}

void Bubble::FadeOut()
{
    // The content window cannot have the layered flag on by default, since its
    // content doesn't always work inside a layered window, but when animating it
    // is ok to set that style on the window for the purpose of fading it out.
    SetWindowLong(GWL_EXSTYLE, GetWindowLong(GWL_EXSTYLE) | WS_EX_LAYERED);
    // This must be the very next call, otherwise we can get flicker on close.
    SetLayeredWindowAttributes(GetNativeView(), 0,
        static_cast<byte>(255), LWA_ALPHA);

    Fade(false); // |fade_in|.
}

void Bubble::Fade(bool fade_in)
{
    animation_.reset(new ui::SlideAnimation(this));
    animation_->SetSlideDuration(kHideFadeDurationMS);
    animation_->SetTweenType(ui::Tween::LINEAR);

    animation_->Reset(fade_in ? 0.0 : 1.0);
    if(fade_in)
    {
        animation_->Show();
    }
    else
    {
        animation_->Hide();
    }
}

bool Bubble::AcceleratorPressed(const view::Accelerator& accelerator)
{
    if(!delegate_ || delegate_->CloseOnEscape())
    {
        DoClose(true);
        return true;
    }
    return false;
}