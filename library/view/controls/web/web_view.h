
#ifndef __view_web_view_h__
#define __view_web_view_h__

#pragma once

#include <atlbase.h>
#include <atlwin.h>

#include <exdisp.h>

#include "base/win/scoped_comptr.h"

#include "view/controls/native_control.h"

namespace view
{

    class WebView : public NativeControl
    {
    public:
        WebView(const std::wstring& navigate_url);
        virtual ~WebView();

        std::wstring navigate_url() const { return navigate_url_; }
        void set_navigate_url(const std::wstring& navigate_url);

    protected:
        // Overridden from NativeControl:
        virtual HWND CreateNativeControl(HWND parent_container);
        virtual LRESULT OnNotify(int w_param, LPNMHDR l_param);

    private:
        CAxWindow native_web_browser_;

        std::wstring navigate_url_;

        mutable base::win::ScopedComPtr<IWebBrowser2> web_browser_;
    };

} //namespace view

#endif //__view_web_view_h__