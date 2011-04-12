
#ifndef __view_native_frame_view_h__
#define __view_native_frame_view_h__

#pragma once

#include "non_client_view.h"

namespace view
{

    class WindowWin;

    class NativeFrameView : public NonClientFrameView
    {
    public:
        explicit NativeFrameView(WindowWin* frame);
        virtual ~NativeFrameView();

        // NonClientFrameView overrides:
        virtual gfx::Rect GetBoundsForClientView() const;
        virtual gfx::Rect GetWindowBoundsForClientBounds(
            const gfx::Rect& client_bounds) const;
        virtual int NonClientHitTest(const gfx::Point& point);
        virtual void GetWindowMask(const gfx::Size& size,
            gfx::Path* window_mask);
        virtual void EnableClose(bool enable);
        virtual void ResetWindowControls();
        virtual void UpdateWindowIcon();

        // View overrides:
        virtual gfx::Size GetPreferredSize();

    private:
        // Our containing frame.
        WindowWin* frame_;

        DISALLOW_COPY_AND_ASSIGN(NativeFrameView);
    };

} //namespace view

#endif //__view_native_frame_view_h__