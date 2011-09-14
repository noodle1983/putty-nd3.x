
#ifndef __contents_container_h__
#define __contents_container_h__

#pragma once

#include "base/memory/scoped_ptr.h"

#include "ui_base/animation/animation_delegate.h"

#include "view/view.h"

namespace ui
{
    class SlideAnimation;
}

namespace view
{
    class Widget;
}

class TabContents;

// ContentsContainer is responsible for managing the TabContents views.
// ContentsContainer has up to two children: one for the currently active
// TabContents and one for instant's TabContents.
class ContentsContainer : public view::View, public ui::AnimationDelegate
{
public:
    // Internal class name
    static const char kViewClassName[];

    explicit ContentsContainer(view::View* active);
    virtual ~ContentsContainer();

    // Makes the preview view the active view and nulls out the old active view.
    // It's assumed the caller will delete or remove the old active view
    // separately.
    void MakePreviewContentsActiveContents();

    // Sets the preview view. This does not delete the old.
    void SetPreview(view::View* preview, TabContents* preview_tab_contents);

    TabContents* preview_tab_contents() const { return preview_tab_contents_; }

    // Sets the active top margin.
    void SetActiveTopMargin(int margin);

    // Returns the bounds of the preview. If the preview isn't active this
    // retuns the bounds the preview would be shown at.
    gfx::Rect GetPreviewBounds();

    // Fades out the active contents.
    void FadeActiveContents();

    // Shows the fade. This is similiar to |FadeActiveContents|, but is immediate.
    void ShowFade();

    // Removes the fade. This is done implicitly when the preview is made active.
    void RemoveFade();

    // View overrides:
    virtual void Layout() OVERRIDE;
    virtual std::string GetClassName() const OVERRIDE;

    // ui::AnimationDelegate overrides:
    virtual void AnimationProgressed(const ui::Animation* animation);

private:
    class OverlayContentView;

    // Creates the overlay widget. The opacity is set at |initial_opacity|.
    void CreateOverlay(int initial_opacity);

    // Invoked when the contents view of the overlay is destroyed.
    void OverlayViewDestroyed();

    view::View* active_;

    view::View* preview_;

    TabContents* preview_tab_contents_;

    // Translucent Widget positioned right above the active view that is used to
    // make the active view appear faded out.
    view::Widget* active_overlay_;

    // Content view of active_overlay. Used to track when the widget is destroyed.
    OverlayContentView* overlay_view_;

    // Animation used to vary the opacity of active_overlay.
    scoped_ptr<ui::SlideAnimation> overlay_animation_;

    // The margin between the top and the active view. This is used to make the
    // preview overlap the bookmark bar on the new tab page.
    int active_top_margin_;

    DISALLOW_COPY_AND_ASSIGN(ContentsContainer);
};

#endif //__contents_container_h__