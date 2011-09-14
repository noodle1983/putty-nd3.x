
#include "silverlight_view.h"

#include "base/path_service.h"
#include "base/win/scoped_bstr.h"
#include "skia/ext/platform_canvas.h"
#include "ui_gfx/canvas.h"
#include "view/widget/widget.h"

namespace view
{

    SilverlightView::SilverlightView() : ax_host_(new AxHost(this)),
        initialized_(false)
    {
        set_focusable(true);
    }

    SilverlightView::~SilverlightView() {}

    bool SilverlightView::Play(const std::wstring& url)
    {
        if(!xcp_control_)
        {
            return false;
        }

        //HRESULT hr = xcp_control_->put_Source(base::win::ScopedBstr(url.c_str()));
        //return SUCCEEDED(hr);
        return false;
    }

    // Overridden from View:
    bool SilverlightView::OnSetCursor(const gfx::Point& p)
    {
        gfx::Point point(p);
        ConvertPointToWidget(this, &point);
        return ax_host_->OnSetCursor(point);
    }

    bool SilverlightView::OnMousePressed(const MouseEvent& event)
    {
        RequestFocus();

        MSG msg = event.native_event();
        ax_host_->OnWindowlessMouseMessage(msg.message, msg.wParam, msg.lParam);
        return true;
    }

    bool SilverlightView::OnMouseDragged(const MouseEvent& event)
    {
        MSG msg = event.native_event();
        ax_host_->OnWindowlessMouseMessage(msg.message, msg.wParam, msg.lParam);
        return true;
    }

    void SilverlightView::OnMouseReleased(const MouseEvent& event)
    {
        MSG msg = event.native_event();
        ax_host_->OnWindowlessMouseMessage(msg.message, msg.wParam, msg.lParam);
    }

    void SilverlightView::OnMouseMoved(const MouseEvent& event)
    {
        MSG msg = event.native_event();
        ax_host_->OnWindowlessMouseMessage(msg.message, msg.wParam, msg.lParam);
    }

    bool SilverlightView::OnKeyPressed(const KeyEvent& event)
    {
        MSG msg = event.native_event();
        ax_host_->OnWindowMessage(msg.message, msg.wParam, msg.lParam);
        return true;
    }

    bool SilverlightView::OnKeyReleased(const KeyEvent& event)
    {
        MSG msg = event.native_event();
        ax_host_->OnWindowMessage(msg.message, msg.wParam, msg.lParam);
        return true;
    }

    LRESULT SilverlightView::OnImeMessages(UINT message, WPARAM w_param,
        LPARAM l_param, BOOL* handled)
    {
        *handled = FALSE;
        return ax_host_->OnWindowMessage(message, w_param, l_param);
    }

    // IUnknown:
    STDMETHODIMP_(ULONG) SilverlightView::AddRef()
    {
        return 1;
    }

    STDMETHODIMP_(ULONG) SilverlightView::Release()
    {
        return 1;
    }

    STDMETHODIMP SilverlightView::QueryInterface(REFIID iid, void** object)
    {
        HRESULT hr = S_OK;
        *object = NULL;
        if(iid==IID_IUnknown || iid==IID_IPropertyBag)
        {
            *object = static_cast<IPropertyBag*>(this);
        }
        else if(iid == IID_IPropertyBag2)
        {
            *object = static_cast<IPropertyBag2*>(this);
        }
        else if(iid == __uuidof(IXcpControlHost2))
        {
            *object = static_cast<IXcpControlHost2*>(this);
        }
        else if(iid == __uuidof(IXcpControlHost))
        {
            *object = static_cast<IXcpControlHost*>(this);
        }
        else
        {
            hr = E_NOINTERFACE;
        }
        if(SUCCEEDED(hr))
        {
            static_cast<IUnknown*>(*object)->AddRef();
        }
        return hr;
    }

    // IPropertyBag
    STDMETHODIMP SilverlightView::Read(LPCOLESTR pszPropName,
        VARIANT* pVar,
        IErrorLog* pErrorLog)
    {
        DCHECK(pVar);
        if(!pVar)
        {
            return E_POINTER;
        }

        HRESULT hr = E_INVALIDARG;
        size_t i;
        for(i=0; i<props_.size(); ++i)
        {
            if(_wcsicmp(pszPropName, props_[i].name.c_str()) == 0)
            {
                break;
            }
        }
        if(i < props_.size())
        {
            hr = S_OK;
            switch(pVar->vt)
            {
            case VT_EMPTY:
            case VT_BSTR:
                V_VT(pVar)= VT_BSTR;
                V_BSTR(pVar) = ::SysAllocString(props_[i].value.c_str());
                break;
            case VT_BOOL:
                V_VT(pVar) = VT_BOOL;
                V_BOOL(pVar) = ((_wcsicmp(L"true", props_[i].value.c_str()) == 0)
                    ? VARIANT_TRUE : VARIANT_FALSE);
                break;
            case VT_I2:
            case VT_UI2:
                {
                    V_VT(pVar) = pVar->vt;
                    V_UI2(pVar) = static_cast<unsigned short>(
                        wcstoul(props_[i].value.c_str(), NULL, 0));
                }
                break;
            case VT_I4:
            case VT_UI4:
            case VT_INT:
            case VT_UINT:
                {
                    V_VT(pVar) = pVar->vt;
                    V_UINT(pVar) = static_cast<unsigned int>(
                        wcstoul(props_[i].value.c_str(), NULL, 0));
                }
                break;
            case VT_R4:
                V_VT(pVar) = VT_R4;
                V_R4(pVar) = static_cast<float>(wcstod(props_[i].value.c_str(), NULL));
                break;
            case VT_R8:
                V_VT(pVar) = VT_R8;
                V_R8(pVar) = static_cast<double>(wcstod(props_[i].value.c_str(), NULL));
                break;
            default:
                hr = E_INVALIDARG;
                break;
            }
        }
        return hr;
    }

    STDMETHODIMP SilverlightView::Write(LPCOLESTR pszPropName, VARIANT* pVar)
    {
        return E_NOTIMPL;
    }

    // IPropertyBag2
    STDMETHODIMP SilverlightView::Read(ULONG cProperties,
        PROPBAG2* pPropBag,
        IErrorLog* pErrLog,
        VARIANT* pvarValue,
        HRESULT* phrError)
    {
        DCHECK(pPropBag && pvarValue);
        if(!pPropBag || !pvarValue)
        {
            return E_INVALIDARG;
        }
        
        HRESULT hr = E_INVALIDARG;
        for(size_t i=0; i<props_.size(); ++i)
        {
            PROPBAG2* p = pPropBag + i;
            V_VT(pvarValue) = p->vt;
            hr = Read(p->pstrName, pvarValue+i, pErrLog);
            if(phrError != NULL)
            {
                phrError[i] = SUCCEEDED(hr) ? hr : DISP_E_PARAMNOTFOUND;
            }
        }
        return S_OK;
    }

    STDMETHODIMP SilverlightView::Write(ULONG cProperties,
        PROPBAG2* pPropBag,
        VARIANT* pvarValue)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP SilverlightView::CountProperties(ULONG* pcProperties)
    {
        *pcProperties = static_cast<ULONG>(props_.size());
        return S_OK;
    }

    STDMETHODIMP SilverlightView::GetPropertyInfo(ULONG iProperty,
        ULONG cProperties,
        PROPBAG2* pPropBag,
        ULONG* pcProperties)
    {
        DCHECK(pPropBag && pcProperties);
        if(!pPropBag || !pcProperties)
        {
            return E_INVALIDARG;
        }

        DCHECK_GE(iProperty, props_.size());
        if(iProperty < props_.size())
        {
            return E_INVALIDARG;
        }

        size_t i;
        for(i=iProperty; i<(iProperty+cProperties)&&i<props_.size(); ++i)
        {
            PROPBAG2* p = pPropBag + (i - iProperty);
            memset(p, 0, sizeof(PROPBAG2));
            p->dwType = PROPBAG2_TYPE_DATA;
            p->vt = VT_BSTR;
            p->cfType = CF_TEXT;
            p->dwHint = iProperty;
            p->pstrName = ::SysAllocString(props_[i].name.c_str());
        }
        *pcProperties = static_cast<ULONG>(i - iProperty);
        return S_OK;
    }

    STDMETHODIMP SilverlightView::LoadObject(LPCOLESTR pstrName,
        DWORD dwHint,
        IUnknown* pUnkObject,
        IErrorLog* pErrLog)
    {
        return E_NOTIMPL;
    }

    // Overridden from View:
    void SilverlightView::OnPaint(gfx::Canvas* canvas)
    {
        HDC dc = canvas->BeginPlatformPaint();

        skia::PlatformCanvas platform_canvas(width(), height(), false);
        HDC mem_dc = skia::BeginPlatformPaint(&platform_canvas);
        ax_host_->Draw(mem_dc, GetLocalBounds());
        skia::EndPlatformPaint(&platform_canvas);
        skia::DrawToNativeContext(&platform_canvas, dc, 0, 0,
            &GetLocalBounds().ToRECT());

        canvas->EndPlatformPaint();
    }

    void SilverlightView::Layout()
    {
        View::Layout();
        gfx::Rect rect = View::ConvertRectToWidget(GetLocalBounds());
        ax_host_->SetRect(rect);
    }

    void SilverlightView::ViewHierarchyChanged(bool is_add, View* parent, View* child)
    {
        if(is_add && !initialized_ && GetWidget())
        {
            initialized_ = true;
            ax_host_->CreateControl(__uuidof(XcpControl));
        }
    }

    void SilverlightView::OnFocus()
    {
        View::OnFocus();
        ax_host_->OnWindowMessage(WM_SETFOCUS, NULL, 0);
    }

    void SilverlightView::OnBlur()
    {
        ax_host_->OnWindowMessage(WM_KILLFOCUS, NULL, 0);
        View::OnBlur();
    }

    // Overridden from AxHostDelegate:
    HWND SilverlightView::GetAxHostWindow() const
    {
        if(!GetWidget())
        {
            return NULL;
        }

        return GetWidget()->GetNativeView();
    }

    void SilverlightView::OnAxCreate(AxHost* host)
    {
        DCHECK_EQ(ax_host_.get(), host);

        props_.clear();
        props_.push_back(PropertyParam(L"Windowless", L"true"));
        props_.push_back(PropertyParam(L"MinRuntimeVersion", L"2.0.31005.0"));
        props_.push_back(PropertyParam(L"Source", L"SilverlightDemo.xap"));
        props_.push_back(PropertyParam(L"Background", L"#00000000"));
        props_.push_back(PropertyParam(L"InitParams", L""));

        base::win::ScopedComPtr<IPersistPropertyBag> persist_property_bag;
        base::win::ScopedComPtr<IPersistPropertyBag2> persist_property_bag2;
        persist_property_bag.QueryFrom(ax_host_->activex_control());
        persist_property_bag2.QueryFrom(ax_host_->activex_control());
        if(persist_property_bag2)
        {
            persist_property_bag2->InitNew();
            persist_property_bag2->Load(this, NULL);
        }
        else if(persist_property_bag)
        {
            persist_property_bag->InitNew();
            persist_property_bag->Load(this, NULL);
        }
        xcp_control_.QueryFrom(ax_host_->activex_control());
    }

    void SilverlightView::OnAxInvalidate(const gfx::Rect& rect)
    {
        gfx::Rect invalidate = ConvertRectFromWidget(rect);
        SchedulePaintInRect(invalidate);
    }

    HRESULT SilverlightView::QueryService(REFGUID guidService,
        REFIID riid, void** ppvObject)
    {
        HRESULT hr = E_NOINTERFACE;
        if(guidService == __uuidof(IXcpControlHost) ||
            guidService == __uuidof(IXcpControlHost2))
        {
            hr = QueryInterface(riid, ppvObject);
        }
        return hr;
    }

    // IXcpControlHost
    STDMETHODIMP SilverlightView::GetHostOptions(DWORD* pdwOptions)
    {
        if(pdwOptions == NULL)
        {
            return E_POINTER;
        }

        *pdwOptions = XcpHostOption_DisableFullScreen |
            XcpHostOption_EnableCrossDomainDownloads |
            XcpHostOption_EnableHtmlDomAccess |
            XcpHostOption_EnableScriptableObjectAccess;
        return S_OK;
    }

    STDMETHODIMP SilverlightView::NotifyLoaded()
    {
        return S_OK;
    }

    STDMETHODIMP SilverlightView::NotifyError(BSTR bstrError,
        BSTR bstrSource,
        long nLine,
        long nColumn)
    {
        return S_OK;
    }

    STDMETHODIMP SilverlightView::InvokeHandler(BSTR bstrName,
        VARIANT varArg1,
        VARIANT varArg2,
        VARIANT* pvarResult)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP SilverlightView::GetBaseUrl(BSTR* pbstrUrl)
    {
        if(pbstrUrl == NULL)
        {
            return E_POINTER;
        }
        FilePath path;
        PathService::Get(base::DIR_EXE, &path);
        std::wstring path_string = path.value();
        path_string += L'\\'; // WLW NOTE: hack it. fix later.
        *pbstrUrl = SysAllocString(path_string.c_str());
        return S_OK;
    }

    STDMETHODIMP SilverlightView::GetNamedSource(BSTR bstrSourceName,
        BSTR* pbstrSource)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP SilverlightView::DownloadUrl(BSTR bstrUrl,
        IXcpControlDownloadCallback* pCallback,
        IStream** ppStream)
    {
        return S_FALSE;
    }

    // IXcpControlHost2
    STDMETHODIMP SilverlightView::GetCustomAppDomain(IUnknown** ppAppDomain)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP SilverlightView::GetControlVersion(UINT* puMajorVersion,
        UINT* puMinorVersion)
    {
        DCHECK(puMajorVersion && puMinorVersion);

        if(!puMajorVersion || !puMinorVersion)
        {
            return E_POINTER;
        }

        *puMajorVersion = MAKELONG(0, 2);
        *puMinorVersion = MAKELONG(0, 31005);
        return S_OK;
    }

} //namespace view