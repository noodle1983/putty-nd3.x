
#include "webbrowser_view.h"

#include "base/win/scoped_variant.h"
#include "ui_gfx/canvas.h"
#include "view/widget/widget.h"

namespace view
{

    WebBrowserView::WebBrowserView() : ax_host_(new AxHost(this)),
        initialized_(false) {}

    WebBrowserView::~WebBrowserView() {}

    bool WebBrowserView::Navigate(const std::wstring& url)
    {
        if(!webbrowser_)
        {
            return false;
        }

        webbrowser_->Navigate2(base::win::ScopedVariant(
            url.c_str()).AsInput(), NULL, NULL, NULL, NULL);
        return true;
    }

    // Overridden from View:
    void WebBrowserView::Layout()
    {
        View::Layout();
        gfx::Rect rect = View::ConvertRectToWidget(GetLocalBounds());
        ax_host_->SetRect(rect);
    }

    void WebBrowserView::ViewHierarchyChanged(bool is_add, View* parent, View* child)
    {
        if(is_add && !initialized_ && GetWidget())
        {
            initialized_ = true;
            ax_host_->CreateControl(__uuidof(WebBrowser));
        }
    }

    void WebBrowserView::VisibilityChanged(View* starting_from, bool is_visible)
    {
        if(!ax_host_.get())
        {
            return ;
        }
        
        base::win::ScopedComPtr<IOleWindow> ole_window;
        ole_window.QueryFrom(ax_host_->activex_control());
        if(!ole_window)
        {
            return ;
        }

        HWND window = NULL;
        ole_window->GetWindow(&window);
        if(!window)
        {
            return ;
        }

        ShowWindow(window, is_visible ? SW_SHOW : SW_HIDE);
    }

    // Overridden from AxHostDelegate:
    HWND WebBrowserView::GetAxHostWindow() const
    {
        if(!GetWidget())
        {
            return NULL;
        }

        return GetWidget()->GetNativeView();
    }

    void WebBrowserView::OnAxCreate(AxHost* host)
    {
        DCHECK_EQ(ax_host_.get(), host);

        webbrowser_.QueryFrom(ax_host_->activex_control());
    }

    void WebBrowserView::OnAxInvalidate(const gfx::Rect& rect) {}

} //namespace view