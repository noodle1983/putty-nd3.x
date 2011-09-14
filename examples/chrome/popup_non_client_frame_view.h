
#ifndef __popup_non_client_frame_view_h__
#define __popup_non_client_frame_view_h__

#pragma once

#include "browser_non_client_frame_view.h"

// BrowserNonClientFrameView implementation for popups. We let the window
// manager implementation render the decorations for popups, so this draws
// nothing.
class PopupNonClientFrameView : public BrowserNonClientFrameView
{
public:
    explicit PopupNonClientFrameView(BrowserFrame* frame);

    // NonClientFrameView:
    virtual gfx::Rect GetBoundsForClientView() const OVERRIDE;
    virtual gfx::Rect GetWindowBoundsForClientBounds(
        const gfx::Rect& client_bounds) const OVERRIDE;
    virtual int NonClientHitTest(const gfx::Point& point);
    virtual void GetWindowMask(const gfx::Size& size,
        gfx::Path* window_mask) OVERRIDE;
    virtual void EnableClose(bool enable) OVERRIDE;
    virtual void ResetWindowControls() OVERRIDE;
    virtual void UpdateWindowIcon() OVERRIDE;

    // BrowserNonClientFrameView:
    virtual gfx::Rect GetBoundsForTabStrip(view::View* tabstrip) const OVERRIDE;
    virtual int GetHorizontalTabStripVerticalOffset(bool restored) const OVERRIDE;
    virtual void UpdateThrobber(bool running) OVERRIDE;

private:
    DISALLOW_COPY_AND_ASSIGN(PopupNonClientFrameView);
};

#endif //__popup_non_client_frame_view_h__