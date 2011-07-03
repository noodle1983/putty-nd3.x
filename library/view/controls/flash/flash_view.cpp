
#include "flash_view.h"

#include "base/win/scoped_bstr.h"

#include "ui_base/win/hwnd_util.h"

using namespace ShockwaveFlashObjects;

namespace
{
    LPCTSTR shockwave_flash_clsid = TEXT("ShockwaveFlash.ShockwaveFlash");
}

namespace view
{

    FlashView::FlashView(const std::wstring& movie_url)
        : movie_url_(movie_url) {}

    FlashView::~FlashView() {}

    void FlashView::set_movie_url(const std::wstring& movie_url)
    {
        if(movie_url_ == movie_url)
        {
            return;
        }

        movie_url_ = movie_url;

        if(shockwave_flash_)
        {
            base::win::ScopedBstr scoped_movie_url(movie_url_.c_str());
            shockwave_flash_->put_Movie(scoped_movie_url);
        }
    }

    HWND FlashView::CreateNativeControl(HWND parent_container)
    {
        native_flash_.Create(parent_container, NULL, shockwave_flash_clsid,
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
        ui::CheckWindowCreated(native_flash_);

        native_flash_.QueryControl(__uuidof(IShockwaveFlash),
            shockwave_flash_.ReceiveVoid());
        if(shockwave_flash_)
        {
            base::win::ScopedBstr scoped_movie_url(movie_url_.c_str());
            shockwave_flash_->put_Movie(scoped_movie_url);
        }
        
        return native_flash_;
    }

    LRESULT FlashView::OnNotify(int w_param, LPNMHDR l_param)
    {
        return 0;
    }

} //namespace view