
#ifndef __wan_chrome_ui_view_tab_abstract_tab_strip_view_h__
#define __wan_chrome_ui_view_tab_abstract_tab_strip_view_h__

#pragma once

#include "view_framework/view/view.h"

// This interface is the way the browser view sees a tab strip's view.
class AbstractTabStripView : public view::View
{
public:
    virtual ~AbstractTabStripView() {}

    // Returns true if the tab strip is editable.
    // Returns false if the tab strip is being dragged or animated to prevent
    // extensions from messing things up while that's happening.
    virtual bool IsTabStripEditable() const = 0;

    // Returns false when there is a drag operation in progress so that the frame
    // doesn't close.
    virtual bool IsTabStripCloseable() const = 0;

    // Updates the loading animations displayed by tabs in the tabstrip to the
    // next frame.
    virtual void UpdateLoadingAnimations() = 0;

    // Returns true if the specified point(TabStrip coordinates) is
    // in the window caption area of the browser window.
    virtual bool IsPositionInWindowCaption(const gfx::Point& point) = 0;

    // Set the background offset used by inactive tabs to match the frame image.
    virtual void SetBackgroundOffset(const gfx::Point& offset) = 0;
};

#endif //__wan_chrome_ui_view_tab_abstract_tab_strip_view_h__