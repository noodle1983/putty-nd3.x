
#include "browser_bubble.h"

#include "view/widget/widget.h"

#include "browser_view.h"

namespace
{

    BrowserBubbleHost* GetBubbleHostFromFrame(view::Widget* frame)
    {
        if(!frame)
        {
            return NULL;
        }

        BrowserBubbleHost* bubble_host = NULL;
        view::Widget* window = frame->GetTopLevelWidget();
        if(window)
        {
            bubble_host = BrowserView::GetBrowserViewForNativeWindow(
                window->GetNativeWindow());
            DCHECK(bubble_host);
        }

        return bubble_host;
    }

}

BrowserBubble::BrowserBubble(view::View* view,
                             view::Widget* frame,
                             const gfx::Rect& relative_to,
                             BubbleBorder::ArrowLocation arrow_location)
                             : frame_(frame),
                             view_(view),
                             relative_to_(relative_to),
                             arrow_location_(arrow_location),
                             delegate_(NULL),
                             attached_(false),
                             bubble_host_(GetBubbleHostFromFrame(frame))
{
    // Keep relative_to_ in frame-relative coordinates to aid in drag
    // positioning.
    gfx::Point origin = relative_to_.origin();
    view::View::ConvertPointToView(NULL, frame_->GetRootView(), &origin);
    relative_to_.set_origin(origin);

    // Use half the corner radius as contents margins so that contents fit better
    // in the bubble. See http://crbug.com/80416.
    int corner_inset = BubbleBorder::GetCornerRadius() / 2;
    gfx::Insets content_margins(corner_inset, corner_inset,
        corner_inset, corner_inset);
    InitPopup(content_margins);
}

BrowserBubble::~BrowserBubble()
{
    DCHECK(!attached_);
    popup_->Close();

    // Don't call DetachFromBrowser from here.  It needs to talk to the
    // BrowserView to deregister itself, and if BrowserBubble is owned
    // by a child of BrowserView, then it's possible that this stack frame
    // is a descendant of BrowserView's destructor, which leads to problems.
    // In that case, Detach doesn't need to get called anyway since BrowserView
    // will do the necessary cleanup.
}

void BrowserBubble::DetachFromBrowser()
{
    DCHECK(attached_);
    if(!attached_)
    {
        return;
    }
    attached_ = false;

    if(bubble_host_)
    {
        bubble_host_->DetachBrowserBubble(this);
    }
}

void BrowserBubble::AttachToBrowser()
{
    DCHECK(!attached_);
    if(attached_)
    {
        return;
    }

    if(bubble_host_)
    {
        bubble_host_->AttachBrowserBubble(this);
    }

    attached_ = true;
}

void BrowserBubble::BrowserWindowMoved()
{
    if(delegate_)
    {
        delegate_->BubbleBrowserWindowMoved(this);
    }
    else
    {
        Hide();
    }
    if(popup_->IsVisible())
    {
        Reposition();
    }
}

void BrowserBubble::BrowserWindowClosing()
{
    if(delegate_)
    {
        delegate_->BubbleBrowserWindowClosing(this);
    }
    else
    {
        Hide();
    }
}

void BrowserBubble::SetBounds(int x, int y, int w, int h)
{
    // If the UI layout is RTL, we don't need to mirror coordinates, since
    // View logic will do that for us.
    bounds_.SetRect(x, y, w, h);
    Reposition();
}

void BrowserBubble::MoveTo(int x, int y)
{
    SetBounds(x, y, bounds_.width(), bounds_.height());
}

void BrowserBubble::Reposition()
{
    gfx::Point top_left;
    view::View::ConvertPointToScreen(frame_->GetRootView(), &top_left);
    MovePopup(top_left.x() + bounds_.x(),
        top_left.y() + bounds_.y(),
        bounds_.width(),
        bounds_.height());
}

gfx::Rect BrowserBubble::GetAbsoluteRelativeTo()
{
    // |relative_to_| is in browser-relative coordinates, so convert it to
    // screen coordinates for use in placing the popup widgets.
    gfx::Rect relative_rect = relative_to_;
    gfx::Point relative_origin = relative_rect.origin();
    view::View::ConvertPointToScreen(frame_->GetRootView(), &relative_origin);
    relative_rect.set_origin(relative_origin);

    return relative_rect;
}

void BrowserBubble::SetAbsoluteBounds(const gfx::Rect& window_bounds)
{
    // Convert screen coordinates to frame relative.
    gfx::Point relative_origin = window_bounds.origin();
    view::View::ConvertPointToView(NULL, frame_->GetRootView(),
        &relative_origin);
    SetBounds(relative_origin.x(), relative_origin.y(),
        window_bounds.width(), window_bounds.height());
}

void BrowserBubble::MovePopup(int x, int y, int w, int h)
{
    popup_->SetBounds(gfx::Rect(x, y, w, h));
}


#include "view/widget/native_widget_win.h"

#include "../bubble/border_content.h"
#include "../bubble/border_widget_win.h"

class BubbleWidget : public view::NativeWidgetWin
{
public:
    explicit BubbleWidget(BrowserBubble* bubble)
        : view::NativeWidgetWin(new view::Widget),
        bubble_(bubble),
        border_widget_(new BorderWidgetWin)
    {
        set_window_style(WS_POPUP | WS_CLIPCHILDREN);
        set_window_ex_style(WS_EX_TOOLWINDOW);
    }

    void ShowAndActivate(bool activate)
    {
        // Show the border first, then the popup overlaid on top.
        border_widget_->Show();
        if(activate)
        {
            ShowWindow(SW_SHOW);
        }
        else
        {
            view::NativeWidgetWin::Show();
        }
    }

    void Close()
    {
        if(!bubble_)
        {
            return; // We have already been closed.
        }
        if(IsActive())
        {
            BrowserBubble::Delegate* delegate = bubble_->delegate();
            if(delegate)
            {
                delegate->BubbleLostFocus(bubble_, NULL);
            }
        }
        border_widget_->Close();
        view::NativeWidgetWin::Close();
        bubble_ = NULL;
    }

    void Hide()
    {
        if(IsActive() && bubble_)
        {
            BrowserBubble::Delegate* delegate = bubble_->delegate();
            if(delegate)
            {
                delegate->BubbleLostFocus(bubble_, NULL);
            }
        }
        view::NativeWidgetWin::Hide();
        border_widget_->Hide();
    }

    void OnActivate(UINT action, BOOL minimized, HWND window)
    {
        NativeWidgetWin::OnActivate(action, minimized, window);
        if(!bubble_)
        {
            return;
        }

        BrowserBubble::Delegate* delegate = bubble_->delegate();
        if(!delegate)
        {
            if(action == WA_INACTIVE)
            {
                bubble_->DetachFromBrowser();
                delete bubble_;
            }
            return;
        }

        if(action == WA_INACTIVE)
        {
            bool lost_focus_to_child = false;

            // Are we a parent of this window?
            HWND parent = window;
            while(parent = ::GetParent(parent))
            {
                if(window == GetNativeView())
                {
                    lost_focus_to_child = true;
                    break;
                }
            }

            // Do we own this window?
            if(!lost_focus_to_child &&
                ::GetWindow(window, GW_OWNER) == GetNativeView())
            {
                lost_focus_to_child = true;
            }

            delegate->BubbleLostFocus(bubble_, lost_focus_to_child);
        }
    }

    virtual void OnSetFocus(HWND focused_window)
    {
        NativeWidgetWin::OnSetFocus(focused_window);
        if(bubble_ && bubble_->delegate())
        {
            bubble_->delegate()->BubbleGotFocus(bubble_);
        }
    }

    BorderWidgetWin* border_widget()
    {
        return border_widget_;
    }

private:
    BrowserBubble* bubble_;
    BorderWidgetWin* border_widget_;

    DISALLOW_COPY_AND_ASSIGN(BubbleWidget);
};

void BrowserBubble::InitPopup(const gfx::Insets& content_margins)
{
    // popup_ is a Widget, but we need to do some NativeWidgetWin stuff first,
    // then we'll assign it into popup_.
    BubbleWidget* bubble_widget = new BubbleWidget(this);

    BorderWidgetWin* border_widget = bubble_widget->border_widget();
    border_widget->InitBorderWidgetWin(new BorderContent,
        frame_->GetNativeView());
    border_widget->border_content()->set_content_margins(content_margins);

    popup_ = bubble_widget->GetWidget();
    // We make the BorderWidgetWin the owner of the Bubble HWND, so that the
    // latter is displayed on top of the former.
    view::Widget::InitParams params(view::Widget::InitParams::TYPE_POPUP);
    params.native_widget = bubble_widget;
    params.parent = border_widget->GetNativeView();
    popup_->Init(params);
    popup_->SetContentsView(view_);

    ResizeToView();
    Reposition();
    AttachToBrowser();
}

void BrowserBubble::Show(bool activate)
{
    if(!popup_->IsVisible())
    {
        static_cast<BubbleWidget*>(popup_->native_widget())->ShowAndActivate(
            activate);
    }
}

void BrowserBubble::Hide()
{
    if(popup_->IsVisible())
    {
        static_cast<BubbleWidget*>(popup_->native_widget())->Hide();
    }
}

void BrowserBubble::ResizeToView()
{
    BorderWidgetWin* border_widget =
        static_cast<BubbleWidget*>(popup_->native_widget())->border_widget();

    gfx::Rect window_bounds;
    window_bounds = border_widget->SizeAndGetBounds(GetAbsoluteRelativeTo(),
        arrow_location_, view_->size());

    SetAbsoluteBounds(window_bounds);
}