
#include "flash_view.h"

#include "base/win/scoped_bstr.h"
#include "ui_gfx/canvas.h"
#include "view/widget/widget.h"

namespace view
{

    FlashView::FlashView() : ax_host_(new AxHost(this)),
        initialized_(false) {}

    FlashView::~FlashView() {}

    bool FlashView::Play(const std::wstring& url)
    {
        if(!flash_)
        {
            return false;
        }

        flash_->Stop();
        flash_->put_Movie(base::win::ScopedBstr(url.c_str()));
        flash_->Play();
        return true;
    }

    // Overridden from View:
    bool FlashView::OnSetCursor(const gfx::Point& p)
    {
        gfx::Point point(p);
        ConvertPointToWidget(this, &point);
        return ax_host_->OnSetCursor(point);
    }

    bool FlashView::OnMousePressed(const MouseEvent& event)
    {
        MSG msg = event.native_event();
        ax_host_->OnWindowlessMouseMessage(msg.message, msg.wParam, msg.lParam);
        return true;
    }

    bool FlashView::OnMouseDragged(const MouseEvent& event)
    {
        MSG msg = event.native_event();
        ax_host_->OnWindowlessMouseMessage(msg.message, msg.wParam, msg.lParam);
        return true;
    }

    void FlashView::OnMouseReleased(const MouseEvent& event)
    {
        MSG msg = event.native_event();
        ax_host_->OnWindowlessMouseMessage(msg.message, msg.wParam, msg.lParam);
    }

    void FlashView::OnMouseMoved(const MouseEvent& event)
    {
        MSG msg = event.native_event();
        ax_host_->OnWindowlessMouseMessage(msg.message, msg.wParam, msg.lParam);
    }

    bool FlashView::OnKeyPressed(const KeyEvent& event)
    {
        MSG msg = event.native_event();
        ax_host_->OnWindowMessage(msg.message, msg.wParam, msg.lParam);
        return true;
    }

    bool FlashView::OnKeyReleased(const KeyEvent& event)
    {
        MSG msg = event.native_event();
        ax_host_->OnWindowMessage(msg.message, msg.wParam, msg.lParam);
        return true;
    }

    // Overridden from View:
    void FlashView::OnPaint(gfx::Canvas* canvas)
    {
        HDC dc = canvas->BeginPlatformPaint();
        ax_host_->Draw(dc, GetLocalBounds());
        canvas->EndPlatformPaint();
    }

    void FlashView::Layout()
    {
        View::Layout();
        gfx::Rect rect = View::ConvertRectToWidget(GetLocalBounds());
        ax_host_->SetRect(rect);
    }

    void FlashView::ViewHierarchyChanged(bool is_add, View* parent, View* child)
    {
        if(is_add && !initialized_ && GetWidget())
        {
            initialized_ = true;
            ax_host_->CreateControl(__uuidof(ShockwaveFlashObjects::ShockwaveFlash));
        }
    }

    // Overridden from AxHostDelegate:
    HWND FlashView::GetAxHostWindow() const
    {
        if(!GetWidget())
        {
            return NULL;
        }

        return GetWidget()->GetNativeView();
    }

    void FlashView::OnAxCreate(AxHost* host)
    {
        DCHECK_EQ(ax_host_.get(), host);

        HRESULT hr = flash_.QueryFrom(ax_host_->activex_control());
        if(flash_)
        {
            hr = flash_->put_WMode(L"Transparent");
        }
    }

    void FlashView::OnAxInvalidate(const gfx::Rect& rect)
    {
        gfx::Rect invalidate = ConvertRectFromWidget(rect);
        SchedulePaintInRect(invalidate);
    }

} //namespace view