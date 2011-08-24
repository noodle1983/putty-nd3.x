
#ifndef __view_native_view_host_win_h__
#define __view_native_view_host_win_h__

#pragma once

#include "base/basic_types.h"

#include "native_view_host_wrapper.h"

namespace view
{

    class NativeViewHost;

    // A Windows implementation of NativeViewHostWrapper
    class NativeViewHostWin : public NativeViewHostWrapper
    {
    public:
        explicit NativeViewHostWin(NativeViewHost* host);
        virtual ~NativeViewHostWin();

        // Overridden from NativeViewHostWrapper:
        virtual void NativeViewAttached();
        virtual void NativeViewDetaching(bool destroyed);
        virtual void AddedToWidget();
        virtual void RemovedFromWidget();
        virtual void InstallClip(int x, int y, int w, int h);
        virtual bool HasInstalledClip();
        virtual void UninstallClip();
        virtual void ShowWidget(int x, int y, int w, int h);
        virtual void HideWidget();
        virtual void SetFocus();
        virtual IAccessible* GetNativeViewAccessible();

    private:
        // Our associated NativeViewHost.
        NativeViewHost* host_;

        // Have we installed a region on the HWND used to clip to only the
        // visible portion of the HWND ?
        bool installed_clip_;

        DISALLOW_COPY_AND_ASSIGN(NativeViewHostWin);
    };

} //namespace view

#endif //__view_native_view_host_win_h__