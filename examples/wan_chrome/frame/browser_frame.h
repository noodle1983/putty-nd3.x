
#ifndef __wan_chrome_frame_browser_frame_h__
#define __wan_chrome_frame_browser_frame_h__

#pragma once

namespace gfx
{
    class Font;
    class Rect;
}

class BrowserView;
class Profile;
class ThemeProvider;

// This is a virtual interface that allows system specific browser frames.
class BrowserFrame
{
public:
    virtual ~BrowserFrame() {}

    // Creates the appropriate BrowserFrame for this platform. The returned
    // object is owned by the caller.
    static BrowserFrame* Create(BrowserView* browser_view, Profile* profile);

    static const gfx::Font& GetTitleFont();

    // Returns the Window associated with this frame. Guraranteed non-NULL after
    // construction.
    virtual view::Window* GetWindow() = 0;

    // Determine the distance of the left edge of the minimize button from the
    // left edge of the window. Used in our Non-Client View's Layout.
    virtual int GetMinimizeButtonOffset() const = 0;

    // Tells the frame to update the throbber.
    virtual void UpdateThrobber(bool running) = 0;

    // Tells the frame to continue a drag detached tab operation.
    virtual void ContinueDraggingDetachedTab() = 0;

    // Returns the theme provider for this frame.
    virtual ThemeProvider* GetThemeProviderForFrame() const = 0;

    // Returns true if the window should use the native frame view. This is true
    // if there are no themes applied on Vista, or if there are themes applied and
    // this browser window is an app or popup.
    virtual bool AlwaysUseNativeFrame() const = 0;

    // Returns the NonClientFrameView of this frame.
    virtual view::View* GetFrameView() const = 0;

    // Paints the shadow edge along the side of the side tabstrip. The BrowserView
    // calls this method _after_ the TabStrip has painted itself so the shadow is
    // rendered above the tabs.
    virtual void PaintTabStripShadow(gfx::Canvas* canvas) = 0;
};

#endif //__wan_chrome_frame_browser_frame_h__