
#include "web_view.h"

#include "base/win/scoped_variant.h"

#include "ui_base/win/hwnd_util.h"

namespace
{
    LPCTSTR web_browser_clsid = TEXT("Shell.Explorer");
}

namespace view
{

    WebView::WebView(const std::wstring& navigate_url)
        : navigate_url_(navigate_url) {}

    WebView::~WebView() {}

    void WebView::set_navigate_url(const std::wstring& navigate_url)
    {
        if(navigate_url_ == navigate_url)
        {
            return;
        }

        navigate_url_ = navigate_url;

        if(web_browser_)
        {
            base::win::ScopedVariant scoped_navigate_url(navigate_url_.c_str());
            web_browser_->Navigate2(scoped_navigate_url.AsInput(),
                NULL, NULL, NULL, NULL);
        }
    }

    HWND WebView::CreateNativeControl(HWND parent_container)
    {
        native_web_browser_.Create(parent_container, NULL, web_browser_clsid,
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
        ui::CheckWindowCreated(native_web_browser_);

        native_web_browser_.QueryControl(__uuidof(IWebBrowser2),
            web_browser_.ReceiveVoid());
        if(web_browser_)
        {
            base::win::ScopedVariant scoped_navigate_url(navigate_url_.c_str());
            web_browser_->Navigate2(scoped_navigate_url.AsInput(), NULL, NULL, NULL, NULL);
        }
        
        return native_web_browser_;
    }

    LRESULT WebView::OnNotify(int w_param, LPNMHDR l_param)
    {
        return 0;
    }

} //namespace view