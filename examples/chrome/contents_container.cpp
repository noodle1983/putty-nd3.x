
#include "contents_container.h"

#include "base/logging.h"

#include "SkColor.h"

#include "ui_base/animation/slide_animation.h"

#include "view/background.h"
#include "view/widget/widget.h"

// Min/max opacity of the overlay.
static const int kMinOpacity = 0;
static const int kMaxOpacity = 192;

// static
const char ContentsContainer::kViewClassName[] = "browser/ContentsContainer";

// View used to track when the overlay widget is destroyed. If the
// ContentsContainer is still valid when the destructor is invoked this invokes
// |OverlayViewDestroyed| on the ContentsContainer.
class ContentsContainer::OverlayContentView : public view::View
{
public:
    explicit OverlayContentView(ContentsContainer* container)
        : container_(container) {}
    ~OverlayContentView()
    {
        if(container_)
        {
            container_->OverlayViewDestroyed();
        }
    }

    void Detach()
    {
        container_ = NULL;
    }

private:
    ContentsContainer* container_;

    DISALLOW_COPY_AND_ASSIGN(OverlayContentView);
};

ContentsContainer::ContentsContainer(view::View* active)
: active_(active),
preview_(NULL),
preview_tab_contents_(NULL),
active_overlay_(NULL),
overlay_view_(NULL),
active_top_margin_(0)
{
    AddChildView(active_);
}

ContentsContainer::~ContentsContainer()
{
    // We don't need to explicitly delete active_overlay_ as it'll be deleted by
    // virtue of being a child window.
    if(overlay_view_)
    {
        overlay_view_->Detach();
    }
}

void ContentsContainer::MakePreviewContentsActiveContents()
{
    DCHECK(preview_);
    RemoveFade();

    active_ = preview_;
    preview_ = NULL;
    preview_tab_contents_ = NULL;
    Layout();
}

void ContentsContainer::SetPreview(view::View* preview,
                                   TabContents* preview_tab_contents)
{
    if(preview == preview_)
    {
        return;
    }

    if(preview_)
    {
        RemoveChildView(preview_);
    }
    preview_ = preview;
    preview_tab_contents_ = preview_tab_contents;
    if(preview_)
    {
        AddChildView(preview_);
    }

    Layout();
}

void ContentsContainer::SetActiveTopMargin(int margin)
{
    if(active_top_margin_ == margin)
    {
        return;
    }

    active_top_margin_ = margin;
    // Make sure we layout next time around. We need this in case our bounds
    // haven't changed.
    InvalidateLayout();
}

gfx::Rect ContentsContainer::GetPreviewBounds()
{
    gfx::Point screen_loc;
    ConvertPointToScreen(this, &screen_loc);
    return gfx::Rect(screen_loc, size());
}

void ContentsContainer::FadeActiveContents()
{
    if(active_overlay_ || !ui::Animation::ShouldRenderRichAnimation())
    {
        return;
    }

    overlay_animation_.reset(new ui::SlideAnimation(this));
    overlay_animation_->SetDuration(300);
    overlay_animation_->SetSlideDuration(300);
    overlay_animation_->Show();

    CreateOverlay(kMinOpacity);
}

void ContentsContainer::ShowFade()
{
    if(active_overlay_ || !ui::Animation::ShouldRenderRichAnimation())
    {
        return;
    }

    CreateOverlay(kMaxOpacity);
}

void ContentsContainer::RemoveFade()
{
    overlay_animation_.reset();
    if(active_overlay_)
    {
        overlay_view_->Detach();
        overlay_view_ = NULL;
        active_overlay_->Close();
        active_overlay_ = NULL;
    }
}

void ContentsContainer::AnimationProgressed(const ui::Animation* animation)
{
    active_overlay_->SetOpacity(
        ui::Tween::ValueBetween(animation->GetCurrentValue(), kMinOpacity,
        kMaxOpacity));
    active_overlay_->GetRootView()->SchedulePaint();
}

void ContentsContainer::Layout()
{
    // The active view always gets the full bounds.
    active_->SetBounds(0, active_top_margin_, width(),
        std::max(0, height() - active_top_margin_));

    if(preview_)
    {
        preview_->SetBounds(0, 0, width(), height());
    }

    // Need to invoke view::View in case any views whose bounds didn't change
    // still need a layout.
    view::View::Layout();
}

std::string ContentsContainer::GetClassName() const
{
    return kViewClassName;
}

void ContentsContainer::CreateOverlay(int initial_opacity)
{
    DCHECK(!active_overlay_);
    gfx::Point screen_origin;
    view::View::ConvertPointToScreen(active_, &screen_origin);
    gfx::Rect overlay_bounds(screen_origin, active_->size());
    view::Widget::InitParams params(view::Widget::InitParams::TYPE_POPUP);
    params.transparent = true;
    params.accept_events = false;
    params.parent = active_->GetWidget()->GetNativeView();
    params.bounds = overlay_bounds;
    active_overlay_ = new view::Widget;
    active_overlay_->Init(params);
    overlay_view_ = new OverlayContentView(this);
    overlay_view_->set_background(
        view::Background::CreateSolidBackground(SK_ColorWHITE));
    active_overlay_->SetContentsView(overlay_view_);
    active_overlay_->SetOpacity(initial_opacity);
    active_overlay_->Show();
    active_overlay_->MoveAboveWidget(active_->GetWidget());
}

void ContentsContainer::OverlayViewDestroyed()
{
    active_overlay_ = NULL;
    overlay_view_ = NULL;
}