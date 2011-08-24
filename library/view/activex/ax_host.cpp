
#include "ax_host.h"

#include <atlbase.h>
#include <atlhost.h>
#include "base/logging.h"
#include "ui_gfx/rect.h"

namespace
{

    class AxFrameWindow : public CWindowImpl<AxFrameWindow>,
        public IOleInPlaceFrame
    {
    public:
        AxFrameWindow() : ref_(1) {}
        virtual ~AxFrameWindow()
        {
            active_object_.Release();
            if(m_hWnd)
            {
                DestroyWindow();
            }
        }

        DECLARE_EMPTY_MSG_MAP()

        // IUnknown:
        STDMETHOD_(ULONG, AddRef)()
        {
            DCHECK_NE(ref_, -1);
            return InterlockedIncrement(&ref_);
        }

        STDMETHOD_(ULONG, Release)()
        {
            LONG ref = InterlockedDecrement(&ref_);
            if(ref == 0)
            {
                delete this;
            }
            return ref;
        }

        STDMETHOD(QueryInterface)(REFIID iid, void** object)
        {
            HRESULT hr = S_OK;
            *object = NULL;
            if(iid==IID_IUnknown || iid==IID_IOleWindow)
            {
                *object = static_cast<IOleWindow*>(this);
            }
            else if(iid == IID_IOleInPlaceUIWindow)
            {
                *object = static_cast<IOleInPlaceUIWindow*>(this);
            }
            else if(iid == IID_IOleInPlaceFrame)
            {
                *object = static_cast<IOleInPlaceFrame*>(this);
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

        // IOleWindow
        STDMETHOD(GetWindow)(HWND* phwnd)
        {
            DCHECK(phwnd);
            if(phwnd == NULL)
            {
                return E_POINTER;
            }

            if(m_hWnd == NULL)
            {
                Create(NULL, NULL, _T("axhost frame window"),
                    WS_OVERLAPPEDWINDOW, 0, (UINT)NULL);
            }
            *phwnd = m_hWnd;
            return S_OK;
        }

        STDMETHOD(ContextSensitiveHelp)(BOOL /*fEnterMode*/)
        {
            return S_OK;
        }

        // IOleInPlaceUIWindow
        STDMETHOD(GetBorder)(LPRECT /*lprectBorder*/)
        {
            return S_OK;
        }

        STDMETHOD(RequestBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
        {
            return INPLACE_E_NOTOOLSPACE;
        }

        STDMETHOD(SetBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
        {
            return S_OK;
        }

        STDMETHOD(SetActiveObject)(IOleInPlaceActiveObject* pActiveObject,
            LPCOLESTR /*pszObjName*/)
        {
            active_object_ = pActiveObject;
            return S_OK;
        }

        // IOleInPlaceFrameWindow
        STDMETHOD(InsertMenus)(HMENU /*hmenuShared*/,
            LPOLEMENUGROUPWIDTHS /*lpMenuWidths*/)
        {
            return S_OK;
        }

        STDMETHOD(SetMenu)(HMENU /*hmenuShared*/,
            HOLEMENU /*holemenu*/,
            HWND /*hwndActiveObject*/)
        {
            return S_OK;
        }

        STDMETHOD(RemoveMenus)(HMENU /*hmenuShared*/)
        {
            return S_OK;
        }

        STDMETHOD(SetStatusText)(LPCOLESTR /*pszStatusText*/)
        {
            return S_OK;
        }

        STDMETHOD(EnableModeless)(BOOL /*fEnable*/)
        {
            return S_OK;
        }

        STDMETHOD(TranslateAccelerator)(LPMSG /*lpMsg*/, WORD /*wID*/)
        {
            return S_FALSE;
        }

        private:
            LONG ref_;
            base::win::ScopedComPtr<IOleInPlaceActiveObject> active_object_;
    };


    class AxUIWindow : public CWindowImpl<AxUIWindow>,
        public IOleInPlaceUIWindow
    {
    public:
        AxUIWindow() : ref_(1) {}
        virtual ~AxUIWindow()
        {
            active_object_.Release();
            if(m_hWnd)
            {
                DestroyWindow();
            }
        }

        DECLARE_EMPTY_MSG_MAP()

        // IUnknown:
        STDMETHOD_(ULONG, AddRef)()
        {
            DCHECK_NE(ref_, -1);
            return InterlockedIncrement(&ref_);
        }

        STDMETHOD_(ULONG, Release)()
        {
            LONG ref = InterlockedDecrement(&ref_);
            if(ref == 0)
            {
                delete this;
            }
            return ref;
        }

        STDMETHOD(QueryInterface)(REFIID iid, void** object)
        {
            HRESULT hr = S_OK;
            *object = NULL;
            if(iid==IID_IUnknown || iid==IID_IOleWindow)
            {
                *object = static_cast<IOleWindow*>(this);
            }
            else if(iid == IID_IOleInPlaceUIWindow)
            {
                *object = static_cast<IOleInPlaceUIWindow*>(this);
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

        // IOleWindow
        STDMETHOD(GetWindow)(HWND* phwnd)
        {
            DCHECK(phwnd);
            if(phwnd == NULL)
            {
                return E_POINTER;
            }

            if(m_hWnd == NULL)
            {
                Create(NULL, NULL, _T("axhost ui window"),
                    WS_OVERLAPPEDWINDOW, 0, (UINT)NULL);
            }
            *phwnd = m_hWnd;
            return S_OK;
        }

        STDMETHOD(ContextSensitiveHelp)(BOOL /*fEnterMode*/)
        {
            return S_OK;
        }

        // IOleInPlaceUIWindow
        STDMETHOD(GetBorder)(LPRECT /*lprectBorder*/)
        {
            return S_OK;
        }

        STDMETHOD(RequestBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
        {
            return INPLACE_E_NOTOOLSPACE;
        }

        STDMETHOD(SetBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
        {
            return S_OK;
        }

        STDMETHOD(SetActiveObject)(IOleInPlaceActiveObject* pActiveObject,
            LPCOLESTR /*pszObjName*/)
        {
            active_object_ = pActiveObject;
            return S_OK;
        }

    private:
        LONG ref_;
        base::win::ScopedComPtr<IOleInPlaceActiveObject> active_object_;
    };

}

namespace view
{

    AxHost::AxHost(AxHostDelegate* delegate)
        : delegate_(delegate),
        screen_dc_(NULL),
        dc_released_(true),
        view_object_type_(0),
        inplace_active_(false),
        ui_active_(false),
        windowless_(false),
        capture_(false),
        have_focus_(false),
        ole_object_sink_(0),
        misc_status_(0),
        accel_(NULL),
        can_windowless_activate_(true),
        user_mode_(true)
    {
        DCHECK(delegate_);
        memset(&bounds_, 0, sizeof(bounds_));
    }

    AxHost::~AxHost()
    {
        ReleaseAll();
    }

    IUnknown* AxHost::controlling_unknown()
    {
        return static_cast<IDispatch*>(this);
    }

    IUnknown* AxHost::activex_control()
    {
        return unknown_.get();
    }

    bool AxHost::CreateControl(const CLSID& clsid)
    {
        ReleaseAll();

        HRESULT hr = unknown_.CreateInstance(clsid);
        if(FAILED(hr))
        {
            return false;
        }

        bool success = ActivateAx();
        if(!success)
        {
            ReleaseAll();
        }

        return success;
    }

    bool AxHost::SetRect(const gfx::Rect& rect)
    {
        if(!::EqualRect(&bounds_, &rect.ToRECT()))
        {
            bounds_ = rect.ToRECT();
            SIZEL pxsize = { bounds_.right-bounds_.left, bounds_.bottom-bounds_.top };
            SIZEL hmsize = { 0 };
            AtlPixelToHiMetric(&pxsize, &hmsize);

            if(ole_object_)
            {
                ole_object_->SetExtent(DVASPECT_CONTENT, &hmsize);
                ole_object_->GetExtent(DVASPECT_CONTENT, &hmsize);
                AtlHiMetricToPixel(&hmsize, &pxsize);
                bounds_.right = bounds_.left + pxsize.cx;
                bounds_.bottom = bounds_.top + pxsize.cy;
            }
            if(inplace_object_windowless_)
            {
                inplace_object_windowless_->SetObjectRects(&bounds_, &bounds_);
            }
            if(windowless_)
            {
                delegate_->OnAxInvalidate(gfx::Rect(bounds_));
            }
        }

        return true;
    }

    void AxHost::Draw(HDC hdc, const gfx::Rect& rect)
    {
        if(view_object_ && windowless_)
        {
            OleDraw(view_object_, DVASPECT_CONTENT, hdc, &rect.ToRECT());
        }
    }

    LRESULT AxHost::OnWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LRESULT lRes = 0;
        if(inplace_active_ && windowless_ && inplace_object_windowless_)
        {
            inplace_object_windowless_->OnWindowMessage(uMsg, wParam, lParam, &lRes);
        }
        return lRes;
    }

    LRESULT AxHost::OnWindowlessMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LRESULT lRes = 0;
        if(inplace_active_ && windowless_ && inplace_object_windowless_)
        {
            inplace_object_windowless_->OnWindowMessage(uMsg, wParam, lParam, &lRes);
        }
        return lRes;
    }

    bool AxHost::OnSetCursor(const gfx::Point& point)
    {
        if(!windowless_)
        {
            return false;
        }

        DWORD dwHitResult = capture_ ? HITRESULT_HIT : HITRESULT_OUTSIDE;
        if(dwHitResult==HITRESULT_OUTSIDE && view_object_)
        {
            view_object_->QueryHitPoint(DVASPECT_CONTENT, &bounds_,
                point.ToPOINT(), 0, &dwHitResult);
        }
        if(dwHitResult == HITRESULT_HIT)
        {
            return true;
        }

        return false;
    }

    // IUnknown:
    STDMETHODIMP_(ULONG) AxHost::AddRef()
    {
        return 1;
    }

    STDMETHODIMP_(ULONG) AxHost::Release()
    {
        return 1;
    }

    STDMETHODIMP AxHost::QueryInterface(REFIID iid, void** object)
    {
        HRESULT hr = S_OK;
        *object = NULL;
        if(iid==IID_IUnknown || iid==IID_IDispatch)
        {
            *object = controlling_unknown();
        }
        else if(iid == IID_IOleClientSite)
        {
            *object = static_cast<IOleClientSite*>(this);
        }
        else if(iid == IID_IOleContainer)
        {
            *object = static_cast<IOleContainer*>(this);
        }
        else if(iid == IID_IOleControlSite)
        {
            *object = static_cast<IOleControlSite*>(this);
        }
        else if(iid == IID_IOleWindow)
        {
            *object = static_cast<IOleWindow*>(this);
        }
        else if(iid == IID_IOleInPlaceSite)
        {
            *object = static_cast<IOleInPlaceSite*>(this);
        }
        else if(iid == IID_IOleInPlaceSiteEx)
        {
            *object = static_cast<IOleInPlaceSiteEx*>(this);
        }
        else if(iid == IID_IOleInPlaceSiteWindowless)
        {
            *object = static_cast<IOleInPlaceSiteWindowless*>(this);
        }
        else if(iid == IID_IAdviseSink)
        {
            *object = static_cast<IAdviseSink*>(this);
        }
        else if(iid == IID_IServiceProvider)
        {
            *object = static_cast<IServiceProvider*>(this);
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

    // IDispatch:
    STDMETHODIMP AxHost::GetTypeInfoCount(UINT* pctinfo)
    {
        if(pctinfo == NULL) 
        {
            return E_POINTER; 
        }
        *pctinfo = 1;
        return S_OK;
    }

    STDMETHODIMP AxHost::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP AxHost::GetIDsOfNames(REFIID riid,
        LPOLESTR* rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID* rgDispId)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP AxHost::Invoke(DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
    {
        HRESULT hr;
        VARIANT varResult;

        if(riid != IID_NULL)
        {
            return E_INVALIDARG;
        }

        if(NULL == pVarResult)
        {
            pVarResult = &varResult;
        }

        VariantInit(pVarResult);

        //set default as boolean
        V_VT(pVarResult)=VT_BOOL;

        // implementation is read-only
        if(!(DISPATCH_PROPERTYGET & wFlags))
        {
            return DISP_E_MEMBERNOTFOUND;
        }

        hr = S_OK;
        switch(dispIdMember)
        {
        case DISPID_AMBIENT_USERMODE:
            V_BOOL(pVarResult)= user_mode_ ? VARIANT_TRUE : VARIANT_FALSE;
            break;

        case DISPID_AMBIENT_UIDEAD:
            V_BOOL(pVarResult)= VARIANT_FALSE;
            break;

        case DISPID_AMBIENT_SUPPORTSMNEMONICS:
            V_BOOL(pVarResult)= VARIANT_FALSE;
            break;

        case DISPID_AMBIENT_SHOWGRABHANDLES:
            V_BOOL(pVarResult)= (!user_mode_) ? VARIANT_TRUE : VARIANT_FALSE;
            break;

        case DISPID_AMBIENT_SHOWHATCHING:
            V_BOOL(pVarResult)= (!user_mode_) ? VARIANT_TRUE : VARIANT_FALSE;
            break;

        default:
            hr = DISP_E_MEMBERNOTFOUND;
            break;
        }
        return hr;
    }

    // IOleClientSite:
    STDMETHODIMP AxHost::SaveObject()
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP AxHost::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker,
        IMoniker** ppmk)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP AxHost::GetContainer(IOleContainer** ppContainer)
    {
        DCHECK(ppContainer);

        HRESULT hr = E_POINTER;
        if(ppContainer)
        {
            hr = E_NOTIMPL;
            (*ppContainer) = NULL;

            if(FAILED(hr))
            {
                hr = QueryInterface(__uuidof(IOleContainer), (void**)ppContainer);
            }
        }

        return hr;
    }

    STDMETHODIMP AxHost::ShowObject()
    {
        delegate_->OnAxInvalidate(gfx::Rect(bounds_));
        return S_OK;
    }

    STDMETHODIMP AxHost::OnShowWindow(BOOL fShow)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP AxHost::RequestNewObjectLayout()
    {
        return E_NOTIMPL;
    }

    // IOleContainer:
    STDMETHODIMP AxHost::ParseDisplayName(IBindCtx* pbc,
        LPOLESTR pszDisplayName,
        ULONG* pchEaten,
        IMoniker** ppmkOut)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP AxHost::EnumObjects(DWORD grfFlags, IEnumUnknown** ppenum)
    {
        if(ppenum == NULL)
        {
            return E_POINTER;
        }
        *ppenum = NULL;

        return E_NOTIMPL;
    }

    STDMETHODIMP AxHost::LockContainer(BOOL fLock)
    {
        return S_OK;
    }

    // IOleControlSite:
    STDMETHODIMP AxHost::OnControlInfoChanged()
    {
        return S_OK;
    }

    STDMETHODIMP AxHost::LockInPlaceActive(BOOL fLock)
    {
        return S_OK;
    }

    STDMETHODIMP AxHost::GetExtendedControl(IDispatch** ppDisp)
    {
        if(ppDisp == NULL)
        {
            return E_POINTER;
        }
        return ole_object_.QueryInterface(ppDisp);
    }

    STDMETHODIMP AxHost::TransformCoords(POINTL* pPtlHimetric,
        POINTF* pPtfContainer,
        DWORD dwFlags)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP AxHost::TranslateAccelerator(MSG* pMsg, DWORD grfModifiers)
    {
        return S_FALSE;
    }

    STDMETHODIMP AxHost::OnFocus(BOOL fGotFocus)
    {
        have_focus_ = fGotFocus;
        return S_OK;
    }

    STDMETHODIMP AxHost::ShowPropertyFrame()
    {
        return E_NOTIMPL;
    }

    // IOleWindow:
    STDMETHODIMP AxHost::GetWindow(HWND* phwnd)
    {
        DCHECK(phwnd);
        *phwnd = delegate_->GetAxHostWindow();
        return S_OK;
    }

    STDMETHODIMP AxHost::ContextSensitiveHelp(BOOL fEnterMode)
    {
        return E_NOTIMPL;
    }

    // IOleInPlaceSite:
    STDMETHODIMP AxHost::CanInPlaceActivate()
    {
        return S_OK;
    }

    STDMETHODIMP AxHost::OnInPlaceActivate()
    {
        DCHECK(!inplace_active_);
        DCHECK(!inplace_object_windowless_);

        inplace_active_ = true;
        OleLockRunning(ole_object_, TRUE, FALSE);
        windowless_ = false;
        ole_object_.QueryInterface(__uuidof(IOleInPlaceObject),
            inplace_object_windowless_.ReceiveVoid());
        return S_OK;
    }

    STDMETHODIMP AxHost::OnUIActivate()
    {
        ui_active_ = true;
        return S_OK;
    }

    STDMETHODIMP AxHost::GetWindowContext(IOleInPlaceFrame** ppFrame,
        IOleInPlaceUIWindow** ppDoc,
        LPRECT lprcPosRect,
        LPRECT lprcClipRect,
        LPOLEINPLACEFRAMEINFO lpFrameInfo)
    {
        if(ppFrame != NULL)
        {
            *ppFrame = NULL;
        }
        if(ppDoc != NULL)
        {
            *ppDoc = NULL;
        }
        if(ppFrame==NULL || ppDoc==NULL || lprcPosRect==NULL || lprcClipRect==NULL)
        {
            NOTREACHED();
            return E_POINTER;
        }

        if(!inplace_frame_)
        {
            inplace_frame_.Attach(new AxFrameWindow());
            DCHECK(inplace_frame_);
        }
        if(!inplace_uiwindow_)
        {
            inplace_uiwindow_.Attach(new AxUIWindow());
            DCHECK(inplace_uiwindow_);
        }
        HRESULT hr = inplace_frame_.QueryInterface(ppFrame);
        if(FAILED(hr))
        {
            return hr;
        }
        hr = inplace_uiwindow_.QueryInterface(ppDoc);
        if(FAILED(hr))
        {
            return hr;
        }

        *lprcPosRect = bounds_;
        *lprcClipRect = bounds_;

        if(accel_ == NULL)
        {
            ACCEL ac = { 0, 0, 0 };
            accel_ = CreateAcceleratorTable(&ac, 1);
        }
        lpFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
        lpFrameInfo->fMDIApp = FALSE;
        lpFrameInfo->hwndFrame = delegate_->GetAxHostWindow();
        lpFrameInfo->haccel = accel_;
        lpFrameInfo->cAccelEntries = (accel_ != NULL) ? 1 : 0;

        return hr;
    }

    STDMETHODIMP AxHost::Scroll(SIZE scrollExtant)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP AxHost::OnUIDeactivate(BOOL fUndoable)
    {
        ui_active_ = false;
        return S_OK;
    }

    STDMETHODIMP AxHost::OnInPlaceDeactivate()
    {
        return OnInPlaceDeactivateEx(TRUE);
    }

    STDMETHODIMP AxHost::DiscardUndoState()
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP AxHost::DeactivateAndUndo()
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP AxHost::OnPosRectChange(LPCRECT lprcPosRect)
    {
        if(lprcPosRect==NULL)
        {
            return E_POINTER;
        }

        return S_OK;
    }

    // IOleInPlaceSiteEx:
    STDMETHODIMP AxHost::OnInPlaceActivateEx(BOOL* /*pfNoRedraw*/, DWORD dwFlags)
    {
        DCHECK(!inplace_active_);
        DCHECK(!inplace_object_windowless_);

        inplace_active_ = true;
        OleLockRunning(ole_object_, TRUE, FALSE);

        HRESULT hr = E_FAIL;
        if(dwFlags & ACTIVATE_WINDOWLESS)
        {
            windowless_ = true;
            hr = inplace_object_windowless_.QueryFrom(ole_object_);
        }
        if(FAILED(hr))
        {
            windowless_ = false;
            hr = ole_object_.QueryInterface(__uuidof(IOleInPlaceObject),
                inplace_object_windowless_.ReceiveVoid());
        }
        if(inplace_object_windowless_)
        {
            inplace_object_windowless_->SetObjectRects(&bounds_, &bounds_);
        }

        return hr;
    }

    STDMETHODIMP AxHost::OnInPlaceDeactivateEx(BOOL fNoRedraw)
    {
        inplace_active_ = false;
        inplace_object_windowless_.Release();
        return S_OK;
    }

    STDMETHODIMP AxHost::RequestUIActivate()
    {
        return S_OK;
    }

    // IOleInPlaceSiteWindowless:
    STDMETHODIMP AxHost::CanWindowlessActivate()
    {
        return can_windowless_activate_ ? S_OK : S_FALSE;
    }

    STDMETHODIMP AxHost::GetCapture()
    {
        return capture_ ? S_OK : S_FALSE;
    }

    STDMETHODIMP AxHost::SetCapture(BOOL fCapture)
    {
        if(fCapture)
        {
            capture_ = true;
        }
        else
        {
            capture_ = false;
        }
        return S_OK;
    }

    STDMETHODIMP AxHost::GetFocus()
    {
        return have_focus_ ? S_OK : S_FALSE;
    }

    STDMETHODIMP AxHost::SetFocus(BOOL fFocus)
    {
        have_focus_ = fFocus;
        return S_OK;
    }

    STDMETHODIMP AxHost::GetDC(LPCRECT pRect, DWORD grfFlags, HDC* phDC)
    {
        if(phDC == NULL)
        {
            return E_POINTER;
        }
        if(!dc_released_)
        {
            return E_FAIL;
        }

        *phDC = ::GetDC(delegate_->GetAxHostWindow());
        if(*phDC == NULL)
        {
            return E_FAIL;
        }

        dc_released_ = false;

        if(grfFlags & OLEDC_NODRAW)
        {
            return S_OK;
        }

        if(grfFlags & OLEDC_OFFSCREEN)
        {
            HDC hDCOffscreen = CreateCompatibleDC(*phDC);
            if(hDCOffscreen != NULL)
            {
                HBITMAP hBitmap = CreateCompatibleBitmap(*phDC,
                    bounds_.right-bounds_.left,
                    bounds_.bottom-bounds_.top);
                if(hBitmap == NULL)
                {
                    DeleteDC(hDCOffscreen);
                }
                else
                {
                    HGDIOBJ hOldBitmap = SelectObject(hDCOffscreen, hBitmap);
                    if(hOldBitmap == NULL)
                    {
                        DeleteObject(hBitmap);
                        DeleteDC(hDCOffscreen);
                    }
                    else
                    {
                        DeleteObject(hOldBitmap);
                        screen_dc_ = *phDC;
                        *phDC = hDCOffscreen;
                    }
                }
            }
        }
        
        return S_OK;
    }

    STDMETHODIMP AxHost::ReleaseDC(HDC hDC)
    {
        dc_released_ = true;
        if(screen_dc_ != NULL)
        {
            // Offscreen DC has to be copied to screen DC before releasing the screen dc;
            BitBlt(screen_dc_, bounds_.left, bounds_.top,
                bounds_.right-bounds_.left,
                bounds_.bottom-bounds_.top,
                hDC, 0, 0, SRCCOPY);
            DeleteDC(hDC);
            hDC = screen_dc_;
        }

        ::ReleaseDC(delegate_->GetAxHostWindow(), hDC);
        return S_OK;
    }

    STDMETHODIMP AxHost::InvalidateRect(LPCRECT pRect, BOOL fErase)
    {
        RECT rect = { 0 };
        if(pRect == NULL)
        {
            rect = bounds_;
        }
        else
        {
            IntersectRect(&rect, pRect, &bounds_);
        }

        if(!IsRectEmpty(&rect))
        {
            delegate_->OnAxInvalidate(gfx::Rect(rect));
        }

        return S_OK;
    }

    STDMETHODIMP AxHost::InvalidateRgn(HRGN hRGN, BOOL fErase)
    {
        if(hRGN == NULL)
        {
            return InvalidateRect(NULL, fErase);
        }

        RECT rc_rgn = { 0 };
        GetRgnBox(hRGN, &rc_rgn);
        return InvalidateRect(&rc_rgn, fErase);
    }

    STDMETHODIMP AxHost::ScrollRect(INT dx, INT dy,
        LPCRECT pRectScroll,
        LPCRECT pRectClip)
    {
        return S_OK;
    }

    STDMETHODIMP AxHost::AdjustRect(LPRECT prc)
    {
        return S_OK;
    }

    STDMETHODIMP AxHost::OnDefWindowMessage(UINT msg,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT* plResult)
    {
        *plResult = DefWindowProc(delegate_->GetAxHostWindow(), msg, wParam, lParam);
        return S_OK;
    }

    // IAdviseSink:
    STDMETHODIMP_(void) AxHost::OnDataChange(FORMATETC* pFormatetc, STGMEDIUM* pStgmed) {}

    STDMETHODIMP_(void) AxHost::OnViewChange(DWORD dwAspect, LONG lindex) {}

    STDMETHODIMP_(void) AxHost::OnRename(IMoniker* pmk) {}

    STDMETHODIMP_(void) AxHost::OnSave() {}

    STDMETHODIMP_(void) AxHost::OnClose() {}

    // IServiceProvider:
    STDMETHODIMP AxHost::QueryService(REFGUID guidService,
        REFIID riid, void** ppvObject)
    {
        DCHECK(ppvObject);
        if(ppvObject == NULL)
        {
            return E_POINTER;
        }
        *ppvObject = NULL;

        HRESULT hr = E_NOINTERFACE;
        // Try for service on this object

        // No services currently
        if(FAILED(hr))
        {
            hr = delegate_->QueryService(guidService, riid, ppvObject);
        }

        return hr;
    }

    // private:
    bool AxHost::ActivateAx()
    {
        ole_object_.QueryFrom(unknown_);
        if(!ole_object_)
        {
            return false;
        }

        ole_object_->GetMiscStatus(DVASPECT_CONTENT, &misc_status_);
        if(misc_status_ & OLEMISC_SETCLIENTSITEFIRST)
        {
            base::win::ScopedComPtr<IOleClientSite> client_site;
            client_site.QueryFrom(controlling_unknown());
            ole_object_->SetClientSite(client_site);
        }

        base::win::ScopedComPtr<IPersistStreamInit> stream_init;
        stream_init.QueryFrom(ole_object_);
        HRESULT hr = stream_init->InitNew();
        if(FAILED(hr))
        {
            // Clean up and return
            if(misc_status_ & OLEMISC_SETCLIENTSITEFIRST)
            {
                ole_object_->SetClientSite(NULL);
            }

            misc_status_ = 0;
            ole_object_.Release();
            unknown_.Release();

            return false;
        }

        if(0 == (misc_status_ & OLEMISC_SETCLIENTSITEFIRST))
        {
            base::win::ScopedComPtr<IOleClientSite> client_site;
            client_site.QueryFrom(controlling_unknown());
            ole_object_->SetClientSite(client_site);
        }

        delegate_->OnAxCreate(this);

        view_object_type_ = 0;
        hr = view_object_.QueryFrom(ole_object_);
        if(FAILED(hr))
        {
            hr = ole_object_.QueryInterface(__uuidof(IViewObject2),
                view_object_.ReceiveVoid());
            if(SUCCEEDED(hr))
            {
                view_object_type_ = 3;
            }
        }
        else
        {
            view_object_type_ = 7;
        }

        if(FAILED(hr))
        {
            hr = ole_object_.QueryInterface(__uuidof(IViewObject),
                view_object_.ReceiveVoid());
            if(SUCCEEDED(hr))
            {
                view_object_type_ = 1;
            }
        }

        base::win::ScopedComPtr<IAdviseSink> advise_sink;
        advise_sink.QueryFrom(controlling_unknown());
        ole_object_->Advise(advise_sink, &ole_object_sink_);
        if(view_object_)
        {
            view_object_->SetAdvise(DVASPECT_CONTENT, 0, this);
        }
        ole_object_->SetHostNames(OLESTR("axhost"), NULL);

        if((misc_status_ & OLEMISC_INVISIBLEATRUNTIME) == 0)
        {
            base::win::ScopedComPtr<IOleClientSite> client_site;
            client_site.QueryFrom(controlling_unknown());
            hr = ole_object_->DoVerb(OLEIVERB_INPLACEACTIVATE,
                NULL, client_site, 0, delegate_->GetAxHostWindow(), &bounds_);
        }

        base::win::ScopedComPtr<IObjectWithSite> site;
        site.QueryFrom(unknown_);
        if(site)
        {
            site->SetSite(controlling_unknown());
        }

        return true;
    }

    void AxHost::ReleaseAll()
    {
        if(view_object_)
        {
            view_object_->SetAdvise(DVASPECT_CONTENT, 0, NULL);
            view_object_.Release();
            view_object_type_ = 0;
        }

        if(ole_object_)
        {
            ole_object_->Unadvise(ole_object_sink_);
            ole_object_->Close(OLECLOSE_NOSAVE);
            ole_object_->SetClientSite(NULL);
            ole_object_.Release();
        }

        if(unknown_)
        {
            base::win::ScopedComPtr<IObjectWithSite> site;
            site.QueryFrom(unknown_);
            if(site)
            {
                site->SetSite(NULL);
            }
            unknown_.Release();
        }

        inplace_object_windowless_.Release();

        inplace_frame_.Release();
        inplace_uiwindow_.Release();

        inplace_active_ = false;
        ui_active_ = false;
        windowless_ = false;
        capture_ = false;
        have_focus_ = false;

        if(accel_ != NULL)
        {
            DestroyAcceleratorTable(accel_);
            accel_ = NULL;
        }
    }

} //namespace view