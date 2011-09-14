
#ifndef __native_browser_frame_h__
#define __native_browser_frame_h__

#pragma once

class BrowserView;

class NativeBrowserFrame
{
public:
    virtual ~NativeBrowserFrame() {}

    // Construct a platform-specific implementation of this interface.
    static NativeBrowserFrame* CreateNativeBrowserFrame(
        BrowserFrame* browser_frame,
        BrowserView* browser_view);

    virtual view::NativeWidget* AsNativeWidget() = 0;
    virtual const view::NativeWidget* AsNativeWidget() const = 0;

protected:
    friend class BrowserFrame;

    // BrowserFrame pass-thrus ---------------------------------------------------
    // See browser_frame.h for documentation:
    virtual int GetMinimizeButtonOffset() const = 0;
    // TODO(beng): replace with some kind of "framechanged" signal to Window.
    virtual void TabStripDisplayModeChanged() = 0;
};

#endif //__native_browser_frame_h__