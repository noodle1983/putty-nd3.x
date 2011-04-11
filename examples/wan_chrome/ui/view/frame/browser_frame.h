
#ifndef __wan_chrome_ui_view_frame_browser_frame_h__
#define __wan_chrome_ui_view_frame_browser_frame_h__

#pragma once

#include "base/basic_types.h"

#include "native_browser_frame_delegate.h"

namespace gfx
{
    class Font;
    class Rect;
}

namespace view
{
    class View;
    class Window;
}
class ThemeProvider;

class AeroGlassNonClientView;
class BrowserNonClientFrameView;
class BrowserRootView;
class BrowserView;
class NativeBrowserFrame;
class NonClientFrameView;
class Profile;

// This is a virtual interface that allows system specific browser frames.
class BrowserFrame : public NativeBrowserFrameDelegate
{
public:
    virtual ~BrowserFrame();

    // Creates the appropriate BrowserFrame for this platform. The returned
    // object is owned by the caller.
    static BrowserFrame* Create(BrowserView* browser_view, Profile* profile);

    static const gfx::Font& GetTitleFont();

    // Returns the Window associated with this frame. Guaranteed non-NULL after
    // construction.
    view::Window* GetWindow();

    // Determine the distance of the left edge of the minimize button from the
    // left edge of the window. Used in our Non-Client View's Layout.
    int GetMinimizeButtonOffset() const;

    // Retrieves the bounds, in non-client view coordinates for the specified
    // TabStrip view.
    gfx::Rect GetBoundsForTabStrip(view::View* tabstrip) const;

    // Returns the y coordinate within the window at which the horizontal TabStrip
    // begins (or would begin).  If |restored| is true, this is calculated as if
    // we were in restored mode regardless of the current mode.
    int GetHorizontalTabStripVerticalOffset(bool restored) const;

    // Tells the frame to update the throbber.
    void UpdateThrobber(bool running);

    // Returns the theme provider for this frame.
    ThemeProvider* GetThemeProviderForFrame() const;

    // Returns true if the window should use the native frame view. This is true
    // if there are no themes applied on Vista, or if there are themes applied and
    // this browser window is an app or popup.
    bool AlwaysUseNativeFrame() const;

    // Returns the NonClientFrameView of this frame.
    view::View* GetFrameView() const;

    // Notifies the frame that the tab strip display mode changed so it can update
    // its frame treatment if necessary.
    void TabStripDisplayModeChanged();

protected:
    // Overridden from NativeBrowserFrameDelegate:
    virtual view::RootView* DelegateCreateRootView();
    virtual view::NonClientFrameView* DelegateCreateFrameViewForWindow();

    // TODO(beng): Temporarily provided as a way to associate the subclass'
    //             implementation of NativeBrowserFrame with this.
    void set_native_browser_frame(NativeBrowserFrame* native_browser_frame)
    {
        native_browser_frame_ = native_browser_frame;
    }

    explicit BrowserFrame(BrowserView* browser_view);

private:
    NativeBrowserFrame* native_browser_frame_;

    // A weak reference to the root view associated with the window. We save a
    // copy as a BrowserRootView to avoid evil casting later, when we need to call
    // functions that only exist on BrowserRootView (versus RootView).
    BrowserRootView* root_view_;

    // A pointer to our NonClientFrameView as a BrowserNonClientFrameView.
    BrowserNonClientFrameView* browser_frame_view_;

    // The BrowserView is our ClientView. This is a pointer to it.
    BrowserView* browser_view_;

    DISALLOW_COPY_AND_ASSIGN(BrowserFrame);
};

#endif //__wan_chrome_ui_view_frame_browser_frame_h__