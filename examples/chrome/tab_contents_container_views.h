
#ifndef __tab_contents_container_views_h__
#define __tab_contents_container_views_h__

#pragma once

#include "view/view.h"

class RenderWidgetHostView;
class TabContents;

class TabContentsContainer : public view::View
{
public:
    // Internal class name
    static const char kViewClassName[];

    TabContentsContainer();
    virtual ~TabContentsContainer();

    // Changes the TabContents associated with this view.
    void ChangeTabContents(TabContents* contents);

    View* GetFocusView() { return this; }

    // Accessor for |tab_contents_|.
    TabContents* tab_contents() const { return tab_contents_; }

    // Called by the BrowserView to notify that |tab_contents| got the focus.
    void TabContentsFocused(TabContents* tab_contents);

    // Tells the container to update less frequently during resizing operations
    // so performance is better.
    void SetFastResize(bool fast_resize);

    // Updates the current reserved rect in view coordinates where contents
    // should not be rendered to draw the resize corner, sidebar mini tabs etc.
    void SetReservedContentsRect(const gfx::Rect& reserved_rect);

    virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

    // Overridden from view::View.
    virtual std::string GetClassName() const OVERRIDE;

private:
    // Add or remove observers for events that we care about.
    void AddObservers();
    void RemoveObservers();

    // Called when a TabContents is destroyed. This gives us a chance to clean
    // up our internal state if the TabContents is somehow destroyed before we
    // get notified.
    void TabContentsDestroyed(TabContents* contents);

    // Called when the RenderWidgetHostView of the hosted TabContents has changed.
    void RenderWidgetHostViewChanged(RenderWidgetHostView* new_view);

    // The attached TabContents.
    TabContents* tab_contents_;

    // The current reserved rect in view coordinates where contents should not be
    // rendered to draw the resize corner, sidebar mini tabs etc.
    // Cached here to update ever changing renderers.
    gfx::Rect cached_reserved_rect_;

    DISALLOW_COPY_AND_ASSIGN(TabContentsContainer);
};

#endif //__tab_contents_container_views_h__