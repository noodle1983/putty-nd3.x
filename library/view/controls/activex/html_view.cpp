
#include "html_view.h"

#include "base/win/scoped_bstr.h"
#include "ui_gfx/canvas.h"
#include "view/widget/widget.h"

namespace view
{

    HtmlView::HtmlView() : ax_host_(new AxHost(this)),
        initialized_(false) {}

    HtmlView::~HtmlView() {}

    bool HtmlView::SetHtml(const std::string& html)
    {
        if(!html_document_)
        {
            return false;
        }

        HRESULT hr = S_OK;
        size_t buf_size = html.size();
        HGLOBAL hGlobal = GlobalAlloc(GHND, buf_size);
        if(hGlobal)
        {
            base::win::ScopedComPtr<IStream> stream;
            BYTE* pBytes = (BYTE*)GlobalLock(hGlobal);
            memcpy(pBytes, html.c_str(), buf_size);
            GlobalUnlock(hGlobal);
            hr = CreateStreamOnHGlobal(hGlobal, TRUE, stream.Receive());
            if(SUCCEEDED(hr))
            {
                base::win::ScopedComPtr<IPersistStreamInit> persist_stream;
                hr = persist_stream.QueryFrom(ax_host_->activex_control());
                if(SUCCEEDED(hr))
                {
                    hr = persist_stream->Load(stream);
                }
            }
        }

        return SUCCEEDED(hr);
    }

    // Overridden from View:
    void HtmlView::Layout()
    {
        View::Layout();
        gfx::Rect rect = View::ConvertRectToWidget(GetLocalBounds());
        ax_host_->SetRect(rect);
    }

    void HtmlView::ViewHierarchyChanged(bool is_add, View* parent, View* child)
    {
        if(is_add && !initialized_ && GetWidget())
        {
            initialized_ = true;
            ax_host_->CreateControl(__uuidof(HTMLDocument));
        }
    }

    void HtmlView::VisibilityChanged(View* starting_from, bool is_visible)
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
    HWND HtmlView::GetAxHostWindow() const
    {
        if(!GetWidget())
        {
            return NULL;
        }

        return GetWidget()->GetNativeView();
    }

    void HtmlView::OnAxCreate(AxHost* host)
    {
        DCHECK_EQ(ax_host_.get(), host);

        html_document_.QueryFrom(ax_host_->activex_control());
    }

    void HtmlView::OnAxInvalidate(const gfx::Rect& rect) {}

} //namespace view