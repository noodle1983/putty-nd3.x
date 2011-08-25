
#ifndef __bubble_h__
#define __bubble_h__

#pragma once

#include "ui_base/animation/animation_delegate.h"

#include "view/widget/native_widget_win.h"

#include "bubble_border.h"

// Bubble is used to display an arbitrary view above all other windows.
// Think of Bubble as a tooltip that allows you to embed an arbitrary view
// in the tooltip. Additionally the Bubble renders an arrow pointing at
// the region the info bubble is providing the information about.
//
// To use an Bubble, invoke Show() and it'll take care of the rest.  The Bubble
// insets the content for you, so the content typically shouldn't have any
// additional margins.

class BorderContent;
class BorderWidgetWin;
class Bubble;

namespace gfx
{
    class Path;
}

namespace ui
{
    class SlideAnimation;
}

namespace view
{
    class Widget;
}

class BubbleDelegate
{
public:
    // Called when the Bubble is closing and is about to be deleted.
    // |closed_by_escape| is true if the close is the result of the user pressing
    // escape.
    virtual void BubbleClosing(Bubble* bubble, bool closed_by_escape) = 0;

    // Whether the Bubble should be closed when the Esc key is pressed.
    virtual bool CloseOnEscape() = 0;

    // Whether the Bubble should fade in when opening. When trying to determine
    // whether to use FadeIn, consider whether the bubble is shown as a direct
    // result of a user action or not. For example, if the bubble is being shown
    // as a direct result of a mouse-click, we should not use FadeIn. However, if
    // the bubble appears as a notification that something happened in the
    // background, we use FadeIn.
    virtual bool FadeInOnShow() = 0;

    // The name of the window to which this delegate belongs.
    virtual std::wstring accessible_name();
};

// TODO(sky): this code is ifdef-tastic. It might be cleaner to refactor the
// WidgetFoo subclass into a separate class that calls into Bubble.
// That way Bubble has no (or very few) ifdefs.
class Bubble : public view::NativeWidgetWin,
    public view::AcceleratorTarget,
    public ui::AnimationDelegate
{
public:
    // Shows the Bubble.
    // |parent| is set as the parent window.
    // |content| are the content shown in the bubble.
    // |position_relative_to| is a rect in screen coordinates at which the Bubble
    // will point.
    // Show() takes ownership of |content| and deletes the created Bubble when
    // another window is activated. You can explicitly close the bubble by
    // invoking Close().
    // |arrow_location| specifies preferred bubble alignment.
    // You may provide an optional |delegate| to:
    //     - Be notified when the Bubble is closed.
    //     - Prevent the Bubble from being closed when the Escape key is
    //       pressed (the default behavior).
    static Bubble* Show(view::Widget* parent,
        const gfx::Rect& position_relative_to,
        BubbleBorder::ArrowLocation arrow_location,
        view::View* content,
        BubbleDelegate* delegate);

    // Resizes and potentially moves the Bubble to best accommodate the
    // content preferred size.
    void SizeToContent();

    // Whether the Bubble should fade away when it closes. Generally speaking,
    // we use FadeOut when the user selects something within the bubble that
    // causes the bubble to dismiss. We don't use it when the bubble gets
    // deactivated as a result of clicking outside the bubble.
    void set_fade_away_on_close(bool fade_away_on_close)
    {
        fade_away_on_close_ = fade_away_on_close;
    }

    // Overridden from NativeWidgetWin:
    virtual void Close();

    // Overridden from ui::AnimationDelegate:
    virtual void AnimationEnded(const ui::Animation* animation);
    virtual void AnimationProgressed(const ui::Animation* animation);

    static const SkColor kBackgroundColor;

protected:
    Bubble();
    virtual ~Bubble();

    // Creates the Bubble.
    virtual void InitBubble(view::Widget* parent,
        const gfx::Rect& position_relative_to,
        BubbleBorder::ArrowLocation arrow_location,
        view::View* content,
        BubbleDelegate* delegate);

    // Instantiates and returns the BorderContent this Bubble should use.
    // Subclasses can return their own BorderContent implementation.
    virtual BorderContent* CreateBorderContent();

    // Overridden from NativeWidgetWin:
    virtual void OnActivate(UINT action, BOOL minimized, HWND window);

    // The window used to render the padding, border and arrow.
    BorderWidgetWin* border_;

private:
    enum ShowStatus
    {
        kOpen,
        kClosing,
        kClosed
    };

    // Closes the window notifying the delegate. |closed_by_escape| is true if
    // the close is the result of pressing escape.
    void DoClose(bool closed_by_escape);

    // Animates to a visible state.
    void FadeIn();
    // Animates to a hidden state.
    void FadeOut();

    // Animates to a visible/hidden state (visible if |fade_in| is true).
    void Fade(bool fade_in);

    void RegisterEscapeAccelerator();
    void UnregisterEscapeAccelerator();

    // Overridden from AcceleratorTarget:
    virtual bool AcceleratorPressed(const view::Accelerator& accelerator);

    // The delegate, if any.
    BubbleDelegate* delegate_;

    // The animation used to fade the bubble out.
    scoped_ptr<ui::SlideAnimation> animation_;

    // The current visibility status of the bubble.
    ShowStatus show_status_;

    // Whether to fade away when the bubble closes.
    bool fade_away_on_close_;

    gfx::Rect position_relative_to_;
    BubbleBorder::ArrowLocation arrow_location_;

    view::View* content_;

    bool accelerator_registered_;

    DISALLOW_COPY_AND_ASSIGN(Bubble);
};

#endif //__bubble_h__