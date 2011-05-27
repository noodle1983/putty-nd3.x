
#include "richview.h"

#include "base/logging.h"

#include "gfx/canvas.h"
#include "gfx/color_utils.h"
#include "gfx/font.h"

#include "../../base/resource_bundle.h"
#include "../../widget/widget.h"

#pragma comment(lib, "riched20.lib")

// 8d33f740-cf58-11ce-a89d-00aa006cadc5
EXTERN_C const IID IID_ITextServices =
{
    0x8d33f740,
    0xcf58,
    0x11ce,
    { 0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5 }
};

// c5bdd8d0-d26e-11ce-a89e-00aa006cadc5
EXTERN_C const IID IID_ITextHost =
{
    0xc5bdd8d0,
    0xd26e,
    0x11ce,
    { 0xa8, 0x9e, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5 }
};

namespace
{

    #define LY_PER_INCH 1440

    HRESULT InitDefaultCharFormat(CHARFORMAT2W& cf)
    {
        gfx::Font font = ResourceBundle::GetSharedInstance().GetFont(
            ResourceBundle::BaseFont);
        LOGFONT lf;
        if(!GetObject(font.GetNativeFont(), sizeof(LOGFONT), &lf))
        {
            return E_FAIL;
        }

        cf.cbSize = sizeof(CHARFORMAT2W);

        HDC screen_ddc = GetDC(GetDesktopWindow());
        int pixels_per_inch = GetDeviceCaps(screen_ddc, LOGPIXELSY);
        cf.yHeight = abs(lf.lfHeight) * LY_PER_INCH / pixels_per_inch;
        ReleaseDC(GetDesktopWindow(), screen_ddc);

        cf.yOffset = 0;
        cf.crTextColor = gfx::GetSysSkColor(COLOR_WINDOWTEXT);

        cf.dwEffects = CFM_EFFECTS | CFE_AUTOBACKCOLOR;
        cf.dwEffects &= ~(CFE_PROTECTED | CFE_LINK);

        if(lf.lfWeight < FW_BOLD)
        {
            cf.dwEffects &= ~CFE_BOLD;
        }
        if(!lf.lfItalic)
        {
            cf.dwEffects &= ~CFE_ITALIC;
        }
        if(!lf.lfUnderline)
        {
            cf.dwEffects &= ~CFE_UNDERLINE;
        }
        if(!lf.lfStrikeOut)
        {
            cf.dwEffects &= ~CFE_STRIKEOUT;
        }

        cf.dwMask = CFM_ALL | CFM_BACKCOLOR;
        cf.bCharSet = lf.lfCharSet;
        cf.bPitchAndFamily = lf.lfPitchAndFamily;
        wcscpy(cf.szFaceName, lf.lfFaceName);

        return S_OK;
    }

    HRESULT InitDefaultParaFormat(PARAFORMAT2& pf)
    {	
        ZeroMemory(&pf, sizeof(PARAFORMAT2));

        pf.cbSize = sizeof(PARAFORMAT2);
        pf.dwMask = PFM_ALL;
        pf.wAlignment = PFA_LEFT;
        pf.cTabCount = 1;
        pf.rgxTabs[0] = lDefaultTab;

        return S_OK;
    }

}

namespace view
{

    static const long kInitTextMax = (32 * 1024) - 1;

    // static
    const char RichView::kViewClassName[] = "view/RichView";

    RichView::RichView(LONG style) : cRefs_(1),
        initialized_(false)
    {
        ZeroMemory(&pserv_, sizeof(RichView)-offsetof(RichView, pserv_));

        cchTextMost_ = kInitTextMax;
        laccelpos_ = -1;
        fTransparent_ = TRUE;
        fRich_ = TRUE;
        dwStyle_ = style;
    }

    RichView::~RichView()
    {
        pserv_->OnTxInPlaceDeactivate();
        pserv_->Release();
    }

    bool RichView::IsReadOnly() const
    {
        return (dwStyle_ & ES_READONLY) != 0;
    }

    void RichView::SetReadOnly(bool read_only)
    {
        if (read_only)
        {
            dwStyle_ |= ES_READONLY;
        }
        else
        {
            dwStyle_ &= ~ES_READONLY;
        }

        // Notify control of property change
        pserv_->OnTxPropertyBitsChange(TXTBIT_READONLY, 
            read_only ? TXTBIT_READONLY : 0);
    }

    bool RichView::IsPassword() const
    {
        return (dwStyle_ & ES_PASSWORD) != 0;
    }

    void RichView::SetPassword(bool password)
    {
        if (password)
        {
            dwStyle_ |= ES_PASSWORD;
        }
        else
        {
            dwStyle_ &= ~ES_PASSWORD;
        }

        // Notify control of property change
        pserv_->OnTxPropertyBitsChange(TXTBIT_USEPASSWORD, 
            password ? TXTBIT_USEPASSWORD : 0);
    }

    bool RichView::IsMultiLine() const
    {
        return (dwStyle_ & ES_MULTILINE) != 0;
    }

    void RichView::SetMultiLine(bool multi_line)
    {
        if (multi_line)
        {
            dwStyle_ |= ES_MULTILINE;
        }
        else
        {
            dwStyle_ &= ~ES_MULTILINE;
        }

        // Notify control of property change
        pserv_->OnTxPropertyBitsChange(TXTBIT_MULTILINE, 
            multi_line ? TXTBIT_WORDWRAP : 0);
    }

    const gfx::Insets& RichView::GetMargins() const
    {
        return margins_;
    }

    void RichView::SetMargins(const gfx::Insets& margins)
    {
        margins_ = margins;

        // Notify control of property change
        pserv_->OnTxPropertyBitsChange(TXTBIT_VIEWINSETCHANGE, 0);
    }

    void RichView::SetDrawBorder(bool draw_border)
    {
    }

    // Overridden from View:

    gfx::Size RichView::GetPreferredSize()
    {
        return gfx::Size(100, 100);
    }

    bool RichView::IsFocusable() const
    {
        return true;
    }

    void RichView::AboutToRequestFocusFromTabTraversal(bool reverse)
    {
    }

    bool RichView::SkipDefaultKeyEventProcessing(const KeyEvent& e)
    {
        return false;
    }

    void RichView::SetEnabled(bool enabled)
    {
        View::SetEnabled(enabled);
    }

    void RichView::OnPaintBackground(gfx::Canvas* canvas)
    {
        
    }

    void RichView::OnPaintFocusBorder(gfx::Canvas* canvas)
    {
        
    }

    void RichView::OnPaint(gfx::Canvas* canvas)
    {
        HDC dc = canvas->BeginPlatformPaint();
        // Remember wparam is actually the hdc and lparam is the update
        // rect because this message has been preprocessed by the window.
        pserv_->TxDraw(
            DVASPECT_CONTENT,  		// Draw Aspect
            /*-1*/0,						// Lindex
            NULL,					// Info for drawing optimazation
            NULL,					// target device information
            dc,			            // Draw device HDC
            NULL, 				   	// Target device HDC
            NULL,			        // Bounding client rectangle
            NULL, 					// Clipping rectangle for metafiles
            NULL,		            // Update rectangle
            NULL, 	   				// Call back function
            NULL,					// Call back parameter
            TXTVIEW_ACTIVE);        // What view of the object
        canvas->EndPlatformPaint();
    }

    bool RichView::OnKeyPressed(const KeyEvent& e)
    {
        const MSG& msg = e.native_event();
        LRESULT lr;
        return pserv_->TxSendMessage(msg.message, msg.wParam, msg.lParam, &lr);
    }

    bool RichView::OnKeyReleased(const KeyEvent& e)
    {
        const MSG& msg = e.native_event();
        LRESULT lr;
        return pserv_->TxSendMessage(msg.message, msg.wParam, msg.lParam, &lr);
    }

    void RichView::OnFocus()
    {

    }

    void RichView::OnBlur()
    {

    }

    // IUnknown实现
    HRESULT RichView::QueryInterface(REFIID riid, void** ppvObject)
    {
        HRESULT hr = E_NOINTERFACE;
        *ppvObject = NULL;

        if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITextHost))
        {
            AddRef();
            *ppvObject = (ITextHost*)this;
            hr = S_OK;
        }

        return hr;
    }

    ULONG RichView::AddRef(void)
    {
        return ++cRefs_;
    }

    ULONG RichView::Release(void)
    {
        ULONG c_Refs = --cRefs_;

        if(c_Refs == 0)
        {
            delete this;
        }

        return c_Refs;
    }


    // ITextHost实现
    HDC RichView::TxGetDC()
    {
        return ::GetDC(GetWidget()->GetNativeView());
    }

    INT RichView::TxReleaseDC(HDC hdc)
    {
        return ::ReleaseDC(GetWidget()->GetNativeView(), hdc);
    }

    BOOL RichView::TxShowScrollBar(INT fnBar, BOOL fShow)
    {
        return FALSE;
    }

    BOOL RichView::TxEnableScrollBar(INT fuSBFlags, INT fuArrowflags)
    {
        return FALSE;
    }

    BOOL RichView::TxSetScrollRange(INT fnBar, LONG nMinPos,
        INT nMaxPos, BOOL fRedraw)
    {
        return FALSE;
    }

    BOOL RichView::TxSetScrollPos(INT fnBar, INT nPos, BOOL fRedraw)
    {
        return FALSE;
    }

    void RichView::TxInvalidateRect(LPCRECT prc, BOOL fMode)
    {
        if(prc)
        {
            SchedulePaintInRect(gfx::Rect(*prc));
        }
        else
        {
            SchedulePaint();
        }
    }

    void RichView::TxViewChange(BOOL fUpdate)
    {
        SchedulePaint();
    }

    BOOL RichView::TxCreateCaret(HBITMAP hbmp, INT xWidth, INT yHeight)
    {
        return ::CreateCaret(GetWidget()->GetNativeView(), hbmp,
            xWidth, yHeight);
    }

    BOOL RichView::TxShowCaret(BOOL fShow)
    {
        if(fShow)
        {
            return ::ShowCaret(GetWidget()->GetNativeView());
        }

        return ::HideCaret(GetWidget()->GetNativeView());
    }

    BOOL RichView::TxSetCaretPos(INT x, INT y)
    {
        return ::SetCaretPos(x, y);
    }

    BOOL RichView::TxSetTimer(UINT idTimer, UINT uTimeout)
    {
        if(GetWidget())
        {
            fTimer_ = TRUE;
            return ::SetTimer(GetWidget()->GetNativeView(), idTimer,
                uTimeout, NULL);
        }
        return FALSE;
    }

    void RichView::TxKillTimer(UINT idTimer)
    {
        if(GetWidget())
        {
            ::KillTimer(GetWidget()->GetNativeView(), idTimer);
            fTimer_ = FALSE;
        }
    }

    void RichView::TxScrollWindowEx(INT dx, INT dy,
        LPCRECT lprcScroll, LPCRECT lprcClip,
        HRGN hrgnUpdate, LPRECT lprcUpdate, UINT fuScroll)
    {
        
    }

    void RichView::TxSetCapture(BOOL fCapture)
    {
        
    }

    void RichView::TxSetFocus()
    {
        Focus();
    }

    void RichView::TxSetCursor(HCURSOR hcur, BOOL fText)
    {
        ::SetCursor(hcur);
    }

    BOOL RichView::TxScreenToClient(LPPOINT lppt)
    {
        return ::ScreenToClient(GetWidget()->GetNativeView(), lppt);
    }

    BOOL RichView::TxClientToScreen(LPPOINT lppt)
    {
        return ::ClientToScreen(GetWidget()->GetNativeView(), lppt);
    }

    HRESULT RichView::TxActivate(LONG* plOldState)
    {
        return S_OK;
    }

    HRESULT RichView::TxDeactivate(LONG lNewState)
    {
        return S_OK;
    }

    HRESULT RichView::TxGetClientRect(LPRECT prc)
    {
        *prc = GetLocalBounds().ToRECT();
        return NOERROR;
    }

    HRESULT RichView::TxGetViewInset(LPRECT prc)
    {
        ::SetRect(prc, margins_.left(), margins_.top(),
            margins_.right(), margins_.bottom());
        return NOERROR;	
    }

    HRESULT RichView::TxGetCharFormat(const CHARFORMATW** ppCF)
    {
        *ppCF = &cf_;
        return NOERROR;
    }

    HRESULT RichView::TxGetParaFormat(const PARAFORMAT** ppPF)
    {
        *ppPF = &pf_;
        return NOERROR;
    }

    COLORREF RichView::TxGetSysColor(int nIndex)
    {
        if(nIndex == COLOR_WINDOW)
        {
            if(!fNotSysBkgnd_)
            {
                return GetSysColor(COLOR_WINDOW);
            }
            return crBackground_;
        }

        return GetSysColor(nIndex);
    }

    HRESULT RichView::TxGetBackStyle(TXTBACKSTYLE* pstyle)
    {
        *pstyle = !fTransparent_ ? TXTBACK_OPAQUE : TXTBACK_TRANSPARENT;
        return NOERROR;
    }

    HRESULT RichView::TxGetMaxLength(DWORD* plength)
    {
        *plength = cchTextMost_;
        return NOERROR;
    }

    HRESULT RichView::TxGetScrollBars(DWORD* pdwScrollBar)
    {
        *pdwScrollBar =  dwStyle_ & (WS_VSCROLL | WS_HSCROLL | ES_AUTOVSCROLL | 
            ES_AUTOHSCROLL | ES_DISABLENOSCROLL);

        return NOERROR;
    }

    HRESULT RichView::TxGetPasswordChar(TCHAR* pch)
    {
        *pch = chPasswordChar_;
        return NOERROR;
    }

    HRESULT RichView::TxGetAcceleratorPos(LONG* pcp)
    {
        *pcp = laccelpos_;
        return S_OK;
    }

    HRESULT RichView::TxGetExtent(LPSIZEL lpExtent)
    {
        return E_NOTIMPL;
    }

    HRESULT RichView::OnTxCharFormatChange(const CHARFORMATW* pcf)
    {
        DCHECK(pcf);
        size_t copy_bytes = std::min(cf_.cbSize, pcf->cbSize);
        memcpy(&cf_, pcf, copy_bytes);

        return S_OK;
    }

    HRESULT RichView::OnTxParaFormatChange(const PARAFORMAT* ppf)
    {
        DCHECK(ppf);
        size_t copy_bytes = std::min(pf_.cbSize, ppf->cbSize);
        memcpy(&pf_, ppf, copy_bytes);

        return S_OK;
    }

    HRESULT RichView::TxGetPropertyBits(DWORD dwMask, DWORD* pdwBits)
    {
        DWORD dwProperties = 0;

        if (fRich_)
        {
            dwProperties = TXTBIT_RICHTEXT;
        }

        if (dwStyle_ & ES_MULTILINE)
        {
            dwProperties |= TXTBIT_MULTILINE;
        }

        if (dwStyle_ & ES_READONLY)
        {
            dwProperties |= TXTBIT_READONLY;
        }


        if (dwStyle_ & ES_PASSWORD)
        {
            dwProperties |= TXTBIT_USEPASSWORD;
        }

        if (!(dwStyle_ & ES_NOHIDESEL))
        {
            dwProperties |= TXTBIT_HIDESELECTION;
        }

        if (fEnableAutoWordSel_)
        {
            dwProperties |= TXTBIT_AUTOWORDSEL;
        }

        if (fVertical_)
        {
            dwProperties |= TXTBIT_VERTICAL;
        }

        if (fWordWrap_)
        {
            dwProperties |= TXTBIT_WORDWRAP;
        }

        if (fAllowBeep_)
        {
            dwProperties |= TXTBIT_ALLOWBEEP;
        }

        if (fSaveSelection_)
        {
            dwProperties |= TXTBIT_SAVESELECTION;
        }

        *pdwBits = dwProperties & dwMask; 
        return NOERROR;
    }

    HRESULT RichView::TxNotify(DWORD iNotify, void* pv)
    {
        return S_OK;
    }

    HIMC RichView::TxImmGetContext()
    {
        return NULL;
    }

    void RichView::TxImmReleaseContext(HIMC himc)
    {
        ImmReleaseContext(GetWidget()->GetNativeView(), himc);
    }

    HRESULT RichView::TxGetSelectionBarWidth(LONG* lSelBarWidth)
    {
        *lSelBarWidth = lSelBarWidth_;
        return S_OK;
    }

    void RichView::ViewHierarchyChanged(bool is_add, View* parent, View* child)
    {
        if(is_add && GetWidget() && !initialized_)
        {
            if(FAILED(InitDefaultCharFormat(cf_)))
            {
                NOTREACHED();
            }

            if(FAILED(InitDefaultParaFormat(pf_)))
            {
                NOTREACHED();
            }

            initialized_ = true;

            if(!(dwStyle_ & (ES_AUTOHSCROLL|WS_HSCROLL)))
            {
                fWordWrap_ = TRUE;
            }

            if(!(dwStyle_ & ES_LEFT))
            {
                if(dwStyle_ & ES_CENTER)
                {
                    pf_.wAlignment = PFA_CENTER;
                }
                else if(dwStyle_ & ES_RIGHT)
                {
                    pf_.wAlignment = PFA_RIGHT;
                }
            }

            fInplaceActive_ = TRUE;

            base::ScopedComPtr<IUnknown> unknown;
            if(FAILED(CreateTextServices(NULL, this, unknown.Receive())))
            {
                NOTREACHED();
            }

            if(FAILED(unknown.QueryInterface(IID_ITextServices, (void**)&pserv_)))
            {
                NOTREACHED();
            }

            // Set window text
            if(FAILED(pserv_->TxSetText(L"我是万连文我是万连文我是万连文我是万连文")))
            {
                NOTREACHED();
            }

            // notify Text Services that we are in place active
            RECT rcClient = GetLocalBounds().ToRECT();
            if(FAILED(pserv_->OnTxInPlaceActivate(&rcClient)))
            {
                NOTREACHED();
            }
        }
    }

    std::string RichView::GetClassName() const
    {
        return kViewClassName;
    }

} //namespace view