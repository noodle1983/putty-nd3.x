
#ifndef __view_flash_view_h__
#define __view_flash_view_h__

#pragma once

#include <atlbase.h>
#include <atlwin.h>

#include "flash10t.tlh"

#include "base/win/scoped_comptr.h"

#include "view/controls/native_control.h"

namespace view
{

    class FlashView : public NativeControl
    {
    public:
        FlashView(const std::wstring& movie_url);
        virtual ~FlashView();

        std::wstring movie_url() const { return movie_url_; }
        void set_movie_url(const std::wstring& movie_url);

    protected:
        // Overridden from NativeControl:
        virtual HWND CreateNativeControl(HWND parent_container);
        virtual LRESULT OnNotify(int w_param, LPNMHDR l_param);

    private:
        CAxWindow native_flash_;

        std::wstring movie_url_;

        mutable base::win::ScopedComPtr<ShockwaveFlashObjects::IShockwaveFlash>
            shockwave_flash_;
    };

} //namespace view

#endif //__view_flash_view_h__