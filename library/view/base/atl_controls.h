
#ifndef __view_framework_atl_richedit_h__
#define __view_framework_atl_richedit_h__

#include <atlbase.h>
#include <atlwin.h>

#include <richedit.h>
#include <richole.h>

///////////////////////////////////////////////////////////////////////////////
// CEditCommands - message handlers for standard EDIT commands

// Chain to CEditCommands message map. Your class must also derive from CEdit.
// Example:
// class CMyEdit : public CWindowImpl<CMyEdit, CEdit>,
//                 public CEditCommands<CMyEdit>
// {
// public:
//      BEGIN_MSG_MAP(CMyEdit)
//              // your handlers...
//              CHAIN_MSG_MAP_ALT(CEditCommands<CMyEdit>, 1)
//      END_MSG_MAP()
//      // other stuff...
// };

template <class T>
class CEditCommands
{
public:
    BEGIN_MSG_MAP(CEditCommands< T >)
        ALT_MSG_MAP(1)
        COMMAND_ID_HANDLER(ID_EDIT_CLEAR, OnEditClear)
        COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, OnEditClearAll)
        COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
        COMMAND_ID_HANDLER(ID_EDIT_CUT, OnEditCut)
        COMMAND_ID_HANDLER(ID_EDIT_PASTE, OnEditPaste)
        COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, OnEditSelectAll)
        COMMAND_ID_HANDLER(ID_EDIT_UNDO, OnEditUndo)
    END_MSG_MAP()

    LRESULT OnEditClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        T* pT = static_cast<T*>(this);
        pT->Clear();
        return 0;
    }

    LRESULT OnEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        T* pT = static_cast<T*>(this);
        pT->SetSel(0, -1);
        pT->Clear();
        return 0;
    }

    LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        T* pT = static_cast<T*>(this);
        pT->Copy();
        return 0;
    }

    LRESULT OnEditCut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        T* pT = static_cast<T*>(this);
        pT->Cut();
        return 0;
    }

    LRESULT OnEditPaste(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        T* pT = static_cast<T*>(this);
        pT->Paste();
        return 0;
    }

    LRESULT OnEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        T* pT = static_cast<T*>(this);
        pT->SetSel(0, -1);
        return 0;
    }

    LRESULT OnEditUndo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        T* pT = static_cast<T*>(this);
        pT->Undo();
        return 0;
    }

    // State (update UI) helpers
    BOOL CanCut() const
    { return HasSelection(); }

    BOOL CanCopy() const
    { return HasSelection(); }

    BOOL CanClear() const
    { return HasSelection(); }

    BOOL CanSelectAll() const
    { return HasText(); }

    BOOL CanFind() const
    { return HasText(); }

    BOOL CanRepeat() const
    { return HasText(); }

    BOOL CanReplace() const
    { return HasText(); }

    BOOL CanClearAll() const
    { return HasText(); }

    // Implementation
    BOOL HasSelection() const
    {
        const T* pT = static_cast<const T*>(this);
        int nMin, nMax;
        ::SendMessage(pT->m_hWnd, EM_GETSEL, (WPARAM)&nMin, (LPARAM)&nMax);
        return (nMin != nMax);
    }

    BOOL HasText() const
    {
        const T* pT = static_cast<const T*>(this);
        return (pT->GetWindowTextLength() > 0);
    }
};


template <class TBase>
class CRichEditCtrlT : public TBase
{
public:
    // Constructors
    CRichEditCtrlT(HWND hWnd = NULL) : TBase(hWnd)
    { }

    CRichEditCtrlT< TBase >& operator =(HWND hWnd)
    {
        m_hWnd = hWnd;
        return *this;
    }

    HWND Create(HWND hWndParent, ATL::_U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
        DWORD dwStyle = 0, DWORD dwExStyle = 0,
        ATL::_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
    {
        return TBase::Create(GetWndClassName(), hWndParent, rect.m_lpRect, szWindowName, dwStyle, dwExStyle, MenuOrID.m_hMenu, lpCreateParam);
    }

    // Attributes
    static LPCTSTR GetWndClassName()
    {
        return RICHEDIT_CLASS;
    }

    static LPCTSTR GetLibraryName()
    {
#if (_RICHEDIT_VER >= 0x0200)
        return _T("RICHED20.DLL");
#else
        return _T("RICHED32.DLL");
#endif
    }

    int GetLineCount() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_GETLINECOUNT, 0, 0L);
    }

    BOOL GetModify() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_GETMODIFY, 0, 0L);
    }

    void SetModify(BOOL bModified = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_SETMODIFY, bModified, 0L);
    }

    void GetRect(LPRECT lpRect) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_GETRECT, 0, (LPARAM)lpRect);
    }

    DWORD GetOptions() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (DWORD)::SendMessage(m_hWnd, EM_GETOPTIONS, 0, 0L);
    }

    DWORD SetOptions(WORD wOperation, DWORD dwOptions)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (DWORD)::SendMessage(m_hWnd, EM_SETOPTIONS, wOperation, dwOptions);
    }

    // NOTE: first word in lpszBuffer must contain the size of the buffer!
    int GetLine(int nIndex, LPTSTR lpszBuffer) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_GETLINE, nIndex, (LPARAM)lpszBuffer);
    }

    int GetLine(int nIndex, LPTSTR lpszBuffer, int nMaxLength) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        *(LPINT)lpszBuffer = nMaxLength;
        return (int)::SendMessage(m_hWnd, EM_GETLINE, nIndex, (LPARAM)lpszBuffer);
    }

    BOOL CanUndo() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_CANUNDO, 0, 0L);
    }

    BOOL CanPaste(UINT nFormat = 0) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_CANPASTE, nFormat, 0L);
    }

    void GetSel(LONG& nStartChar, LONG& nEndChar) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        CHARRANGE cr = { 0, 0 };
        ::SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
        nStartChar = cr.cpMin;
        nEndChar = cr.cpMax;
    }

    void GetSel(CHARRANGE &cr) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
    }

    int SetSel(LONG nStartChar, LONG nEndChar)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        CHARRANGE cr = { nStartChar, nEndChar };
        return (int)::SendMessage(m_hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);
    }

    int SetSel(CHARRANGE &cr)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);
    }

    int SetSelAll()
    {
        return SetSel(0, -1);
    }

    int SetSelNone()
    {
        return SetSel(-1, 0);
    }

    DWORD GetDefaultCharFormat(CHARFORMAT& cf) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT);
        return (DWORD)::SendMessage(m_hWnd, EM_GETCHARFORMAT, 0, (LPARAM)&cf);
    }

    DWORD GetSelectionCharFormat(CHARFORMAT& cf) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT);
        return (DWORD)::SendMessage(m_hWnd, EM_GETCHARFORMAT, 1, (LPARAM)&cf);
    }

    DWORD GetEventMask() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (DWORD)::SendMessage(m_hWnd, EM_GETEVENTMASK, 0, 0L);
    }

    LONG GetLimitText() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (LONG)::SendMessage(m_hWnd, EM_GETLIMITTEXT, 0, 0L);
    }

    DWORD GetParaFormat(PARAFORMAT& pf) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        pf.cbSize = sizeof(PARAFORMAT);
        return (DWORD)::SendMessage(m_hWnd, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
    }

#if (_RICHEDIT_VER >= 0x0200)
    LONG GetSelText(LPTSTR lpstrBuff) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (LONG)::SendMessage(m_hWnd, EM_GETSELTEXT, 0, (LPARAM)lpstrBuff);
    }
#else // !(_RICHEDIT_VER >= 0x0200)
    // RichEdit 1.0 EM_GETSELTEXT is ANSI only
    LONG GetSelText(LPSTR lpstrBuff) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (LONG)::SendMessage(m_hWnd, EM_GETSELTEXT, 0, (LPARAM)lpstrBuff);
    }
#endif // !(_RICHEDIT_VER >= 0x0200)

    BOOL GetSelTextBSTR(BSTR& bstrText) const
    {
        USES_CONVERSION;
        ATLASSERT(::IsWindow(m_hWnd));
        ATLASSERT(bstrText == NULL);

        CHARRANGE cr = { 0, 0 };
        ::SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);

#if (_RICHEDIT_VER >= 0x0200)
        LPTSTR lpstrText = (LPTSTR)_alloca((cr.cpMax - cr.cpMin + 1) * sizeof(TCHAR));
        lpstrText[0] = 0;
        if(::SendMessage(m_hWnd, EM_GETSELTEXT, 0, (LPARAM)lpstrText) == 0)
            return FALSE;

        bstrText = ::SysAllocString(T2W(lpstrText));
#else // !(_RICHEDIT_VER >= 0x0200)
        LPSTR lpstrText = (char*)_alloca(cr.cpMax - cr.cpMin + 1);
        lpstrText[0] = 0;
        if(::SendMessage(m_hWnd, EM_GETSELTEXT, 0, (LPARAM)lpstrText) == 0)
            return FALSE;

        bstrText = ::SysAllocString(A2W(lpstrText));
#endif // !(_RICHEDIT_VER >= 0x0200)

        return (bstrText != NULL) ? TRUE : FALSE;
    }

#if defined(_WTL_USE_CSTRING) || defined(__ATLSTR_H__)
    LONG GetSelText(_CSTRING_NS::CString& strText) const
    {
        ATLASSERT(::IsWindow(m_hWnd));

        CHARRANGE cr = { 0, 0 };
        ::SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);

#if (_RICHEDIT_VER >= 0x0200)
        LONG lLen = 0;
        LPTSTR lpstrText = strText.GetBufferSetLength(cr.cpMax - cr.cpMin);
        if(lpstrText != NULL)
        {
            lLen = (LONG)::SendMessage(m_hWnd, EM_GETSELTEXT, 0, (LPARAM)lpstrText);
            strText.ReleaseBuffer();
        }
#else // !(_RICHEDIT_VER >= 0x0200)
        LPSTR lpstrText = (char*)_alloca(cr.cpMax - cr.cpMin + 1);
        lpstrText[0] = 0;
        LONG lLen = (LONG)::SendMessage(m_hWnd, EM_GETSELTEXT, 0, (LPARAM)lpstrText);
        if(lLen == 0)
            return 0;

        USES_CONVERSION;
        strText = A2T(lpstrText);
#endif // !(_RICHEDIT_VER >= 0x0200)

        return lLen;
    }
#endif // defined(_WTL_USE_CSTRING) || defined(__ATLSTR_H__)

    WORD GetSelectionType() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (WORD)::SendMessage(m_hWnd, EM_SELECTIONTYPE, 0, 0L);
    }

    COLORREF SetBackgroundColor(COLORREF cr)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (COLORREF)::SendMessage(m_hWnd, EM_SETBKGNDCOLOR, 0, cr);
    }

    COLORREF SetBackgroundColor()   // sets to system background
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (COLORREF)::SendMessage(m_hWnd, EM_SETBKGNDCOLOR, 1, 0);
    }

    BOOL SetCharFormat(CHARFORMAT& cf, WORD wFlags)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT);
        return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, (WPARAM)wFlags, (LPARAM)&cf);
    }

    BOOL SetDefaultCharFormat(CHARFORMAT& cf)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT);
        return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, 0, (LPARAM)&cf);
    }

    BOOL SetSelectionCharFormat(CHARFORMAT& cf)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT);
        return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    }

    BOOL SetWordCharFormat(CHARFORMAT& cf)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT);
        return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD, (LPARAM)&cf);
    }

    DWORD SetEventMask(DWORD dwEventMask)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (DWORD)::SendMessage(m_hWnd, EM_SETEVENTMASK, 0, dwEventMask);
    }

    BOOL SetParaFormat(PARAFORMAT& pf)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        pf.cbSize = sizeof(PARAFORMAT);
        return (BOOL)::SendMessage(m_hWnd, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
    }

    BOOL SetTargetDevice(HDC hDC, int cxLineWidth)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_SETTARGETDEVICE, (WPARAM)hDC, cxLineWidth);
    }

    int GetTextLength() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0L);
    }

    BOOL SetReadOnly(BOOL bReadOnly = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_SETREADONLY, bReadOnly, 0L);
    }

    int GetFirstVisibleLine() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_GETFIRSTVISIBLELINE, 0, 0L);
    }

    EDITWORDBREAKPROCEX GetWordBreakProcEx() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (EDITWORDBREAKPROCEX)::SendMessage(m_hWnd, EM_GETWORDBREAKPROCEX, 0, 0L);
    }

    EDITWORDBREAKPROCEX SetWordBreakProcEx(EDITWORDBREAKPROCEX pfnEditWordBreakProcEx)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (EDITWORDBREAKPROCEX)::SendMessage(m_hWnd, EM_SETWORDBREAKPROCEX, 0, (LPARAM)pfnEditWordBreakProcEx);
    }

    int GetTextRange(TEXTRANGE* pTextRange) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_GETTEXTRANGE, 0, (LPARAM)pTextRange);
    }

#if (_RICHEDIT_VER >= 0x0200)
    int GetTextRange(LONG nStartChar, LONG nEndChar, LPTSTR lpstrText) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        TEXTRANGE tr = { 0 };
        tr.chrg.cpMin = nStartChar;
        tr.chrg.cpMax = nEndChar;
        tr.lpstrText = lpstrText;
        return (int)::SendMessage(m_hWnd, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
    }
#else // !(_RICHEDIT_VER >= 0x0200)

    int GetTextRange(LONG nStartChar, LONG nEndChar, LPSTR lpstrText) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        TEXTRANGE tr = { 0 };
        tr.chrg.cpMin = nStartChar;
        tr.chrg.cpMax = nEndChar;
        tr.lpstrText = lpstrText;
        return (int)::SendMessage(m_hWnd, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
    }
#endif // !(_RICHEDIT_VER >= 0x0200)

#if (_RICHEDIT_VER >= 0x0200)
    DWORD GetDefaultCharFormat(CHARFORMAT2& cf) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT2);
        return (DWORD)::SendMessage(m_hWnd, EM_GETCHARFORMAT, 0, (LPARAM)&cf);
    }

    BOOL SetCharFormat(CHARFORMAT2& cf, WORD wFlags)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT2);
        return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, (WPARAM)wFlags, (LPARAM)&cf);
    }

    BOOL SetDefaultCharFormat(CHARFORMAT2& cf)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT2);
        return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, 0, (LPARAM)&cf);
    }

    DWORD GetSelectionCharFormat(CHARFORMAT2& cf) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT2);
        return (DWORD)::SendMessage(m_hWnd, EM_GETCHARFORMAT, 1, (LPARAM)&cf);
    }

    BOOL SetSelectionCharFormat(CHARFORMAT2& cf)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT2);
        return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    }

    BOOL SetWordCharFormat(CHARFORMAT2& cf)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        cf.cbSize = sizeof(CHARFORMAT2);
        return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD, (LPARAM)&cf);
    }

    DWORD GetParaFormat(PARAFORMAT2& pf) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        pf.cbSize = sizeof(PARAFORMAT2);
        return (DWORD)::SendMessage(m_hWnd, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
    }

    BOOL SetParaFormat(PARAFORMAT2& pf)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        pf.cbSize = sizeof(PARAFORMAT2);
        return (BOOL)::SendMessage(m_hWnd, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
    }

    TEXTMODE GetTextMode() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (TEXTMODE)::SendMessage(m_hWnd, EM_GETTEXTMODE, 0, 0L);
    }

    BOOL SetTextMode(TEXTMODE enumTextMode)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return !(BOOL)::SendMessage(m_hWnd, EM_SETTEXTMODE, enumTextMode, 0L);
    }

    UNDONAMEID GetUndoName() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (UNDONAMEID)::SendMessage(m_hWnd, EM_GETUNDONAME, 0, 0L);
    }

    UNDONAMEID GetRedoName() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (UNDONAMEID)::SendMessage(m_hWnd, EM_GETREDONAME, 0, 0L);
    }

    BOOL CanRedo() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_CANREDO, 0, 0L);
    }

    BOOL GetAutoURLDetect() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_GETAUTOURLDETECT, 0, 0L);
    }

    BOOL SetAutoURLDetect(BOOL bAutoDetect = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return !(BOOL)::SendMessage(m_hWnd, EM_AUTOURLDETECT, bAutoDetect, 0L);
    }

    // this method is deprecated, please use SetAutoURLDetect
    BOOL EnableAutoURLDetect(BOOL bEnable = TRUE) { return SetAutoURLDetect(bEnable); }

    UINT SetUndoLimit(UINT uUndoLimit)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (UINT)::SendMessage(m_hWnd, EM_SETUNDOLIMIT, uUndoLimit, 0L);
    }

    void SetPalette(HPALETTE hPalette)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_SETPALETTE, (WPARAM)hPalette, 0L);
    }

    int GetTextEx(GETTEXTEX* pGetTextEx, LPTSTR lpstrText) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_GETTEXTEX, (WPARAM)pGetTextEx, (LPARAM)lpstrText);
    }

    int GetTextEx(LPTSTR lpstrText, int nTextLen, DWORD dwFlags = GT_DEFAULT, UINT uCodePage = CP_ACP, LPCSTR lpDefaultChar = NULL, LPBOOL lpUsedDefChar = NULL) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        GETTEXTEX gte = { 0 };
        gte.cb = nTextLen * sizeof(TCHAR);
        gte.codepage = uCodePage;
        gte.flags = dwFlags;
        gte.lpDefaultChar = lpDefaultChar;
        gte.lpUsedDefChar = lpUsedDefChar;
        return (int)::SendMessage(m_hWnd, EM_GETTEXTEX, (WPARAM)&gte, (LPARAM)lpstrText);
    }

    int GetTextLengthEx(GETTEXTLENGTHEX* pGetTextLengthEx) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_GETTEXTLENGTHEX, (WPARAM)pGetTextLengthEx, 0L);
    }

    int GetTextLengthEx(DWORD dwFlags = GTL_DEFAULT, UINT uCodePage = CP_ACP) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        GETTEXTLENGTHEX gtle = { 0 };
        gtle.codepage = uCodePage;
        gtle.flags = dwFlags;
        return (int)::SendMessage(m_hWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtle, 0L);
    }
#endif // (_RICHEDIT_VER >= 0x0200)

#if (_RICHEDIT_VER >= 0x0300)
    int SetTextEx(SETTEXTEX* pSetTextEx, LPCTSTR lpstrText)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_SETTEXTEX, (WPARAM)pSetTextEx, (LPARAM)lpstrText);
    }

    int SetTextEx(LPCTSTR lpstrText, DWORD dwFlags = ST_DEFAULT, UINT uCodePage = CP_ACP)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        SETTEXTEX ste = { 0 };
        ste.flags = dwFlags;
        ste.codepage = uCodePage;
        return (int)::SendMessage(m_hWnd, EM_SETTEXTEX, (WPARAM)&ste, (LPARAM)lpstrText);
    }

    int GetEditStyle() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_GETEDITSTYLE, 0, 0L);
    }

    int SetEditStyle(int nStyle, int nMask = -1)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        if(nMask == -1)
            nMask = nStyle;   // set everything specified
        return (int)::SendMessage(m_hWnd, EM_SETEDITSTYLE, nStyle, nMask);
    }

    BOOL SetFontSize(int nFontSizeDelta)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ATLASSERT(nFontSizeDelta >= -1637 && nFontSizeDelta <= 1638);
        return (BOOL)::SendMessage(m_hWnd, EM_SETFONTSIZE, nFontSizeDelta, 0L);
    }

    void GetScrollPos(LPPOINT lpPoint) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ATLASSERT(lpPoint != NULL);
        ::SendMessage(m_hWnd, EM_GETSCROLLPOS, 0, (LPARAM)lpPoint);
    }

    void SetScrollPos(LPPOINT lpPoint)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ATLASSERT(lpPoint != NULL);
        ::SendMessage(m_hWnd, EM_SETSCROLLPOS, 0, (LPARAM)lpPoint);
    }

    BOOL GetZoom(int& nNum, int& nDen) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_GETZOOM, (WPARAM)&nNum, (LPARAM)&nDen);
    }

    BOOL SetZoom(int nNum, int nDen)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ATLASSERT(nNum >= 0 && nNum <= 64);
        ATLASSERT(nDen >= 0 && nDen <= 64);
        return (BOOL)::SendMessage(m_hWnd, EM_SETZOOM, nNum, nDen);
    }

    BOOL SetZoomOff()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_SETZOOM, 0, 0L);
    }
#endif // (_RICHEDIT_VER >= 0x0300)

    // Operations
    void LimitText(LONG nChars = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_EXLIMITTEXT, 0, nChars);
    }

    int LineFromChar(LONG nIndex) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_EXLINEFROMCHAR, 0, nIndex);
    }

    POINT PosFromChar(LONG nChar) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        POINT point = { 0, 0 };
        ::SendMessage(m_hWnd, EM_POSFROMCHAR, (WPARAM)&point, nChar);
        return point;
    }

    int CharFromPos(POINT pt) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        POINTL ptl = { pt.x, pt.y };
        return (int)::SendMessage(m_hWnd, EM_CHARFROMPOS, 0, (LPARAM)&ptl);
    }

    void EmptyUndoBuffer()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_EMPTYUNDOBUFFER, 0, 0L);
    }

    int LineIndex(int nLine = -1) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_LINEINDEX, nLine, 0L);
    }

    int LineLength(int nLine = -1) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (int)::SendMessage(m_hWnd, EM_LINELENGTH, nLine, 0L);
    }

    BOOL LineScroll(int nLines, int nChars = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_LINESCROLL, nChars, nLines);
    }

    void ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo = FALSE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_REPLACESEL, (WPARAM) bCanUndo, (LPARAM)lpszNewText);
    }

    void SetRect(LPCRECT lpRect)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_SETRECT, 0, (LPARAM)lpRect);
    }

    BOOL DisplayBand(LPRECT pDisplayRect)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_DISPLAYBAND, 0, (LPARAM)pDisplayRect);
    }

    LONG FindText(DWORD dwFlags, FINDTEXT& ft) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
#if (_RICHEDIT_VER >= 0x0200) && defined(_UNICODE)
        return (LONG)::SendMessage(m_hWnd, EM_FINDTEXTW, dwFlags, (LPARAM)&ft);
#else
        return (LONG)::SendMessage(m_hWnd, EM_FINDTEXT, dwFlags, (LPARAM)&ft);
#endif
    }

    LONG FindText(DWORD dwFlags, FINDTEXTEX& ft) const
    {
        ATLASSERT(::IsWindow(m_hWnd));
#if (_RICHEDIT_VER >= 0x0200) && defined(_UNICODE)
        return (LONG)::SendMessage(m_hWnd, EM_FINDTEXTEXW, dwFlags, (LPARAM)&ft);
#else
        return (LONG)::SendMessage(m_hWnd, EM_FINDTEXTEX, dwFlags, (LPARAM)&ft);
#endif
    }

    LONG FormatRange(FORMATRANGE& fr, BOOL bDisplay = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (LONG)::SendMessage(m_hWnd, EM_FORMATRANGE, bDisplay, (LPARAM)&fr);
    }

    LONG FormatRange(FORMATRANGE* pFormatRange, BOOL bDisplay = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (LONG)::SendMessage(m_hWnd, EM_FORMATRANGE, bDisplay, (LPARAM)pFormatRange);
    }

    void HideSelection(BOOL bHide = TRUE, BOOL bChangeStyle = FALSE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_HIDESELECTION, bHide, bChangeStyle);
    }

    void PasteSpecial(UINT uClipFormat, DWORD dwAspect = 0, HMETAFILE hMF = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        REPASTESPECIAL reps = { dwAspect, (DWORD_PTR)hMF };
        ::SendMessage(m_hWnd, EM_PASTESPECIAL, uClipFormat, (LPARAM)&reps);
    }

    void RequestResize()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_REQUESTRESIZE, 0, 0L);
    }

    LONG StreamIn(UINT uFormat, EDITSTREAM& es)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (LONG)::SendMessage(m_hWnd, EM_STREAMIN, uFormat, (LPARAM)&es);
    }

    LONG StreamOut(UINT uFormat, EDITSTREAM& es)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (LONG)::SendMessage(m_hWnd, EM_STREAMOUT, uFormat, (LPARAM)&es);
    }

    DWORD FindWordBreak(int nCode, LONG nStartChar)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (DWORD)::SendMessage(m_hWnd, EM_FINDWORDBREAK, nCode, nStartChar);
    }

    // Additional operations
    void ScrollCaret()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_SCROLLCARET, 0, 0L);
    }

    int InsertText(long nInsertAfterChar, LPCTSTR lpstrText, BOOL bCanUndo = FALSE)
    {
        int nRet = SetSel(nInsertAfterChar, nInsertAfterChar);
        ReplaceSel(lpstrText, bCanUndo);
        return nRet;
    }

    int AppendText(LPCTSTR lpstrText, BOOL bCanUndo = FALSE)
    {
        return InsertText(GetWindowTextLength(), lpstrText, bCanUndo);
    }

    // Clipboard operations
    BOOL Undo()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_UNDO, 0, 0L);
    }

    void Clear()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_CLEAR, 0, 0L);
    }

    void Copy()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_COPY, 0, 0L);
    }

    void Cut()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_CUT, 0, 0L);
    }

    void Paste()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_PASTE, 0, 0L);
    }

    // OLE support
    IRichEditOle* GetOleInterface() const
    {
        ATLASSERT(::IsWindow(m_hWnd));
        IRichEditOle *pRichEditOle = NULL;
        ::SendMessage(m_hWnd, EM_GETOLEINTERFACE, 0, (LPARAM)&pRichEditOle);
        return pRichEditOle;
    }

    BOOL SetOleCallback(IRichEditOleCallback* pCallback)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_SETOLECALLBACK, 0, (LPARAM)pCallback);
    }

#if (_RICHEDIT_VER >= 0x0200)
    BOOL Redo()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_REDO, 0, 0L);
    }

    void StopGroupTyping()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_STOPGROUPTYPING, 0, 0L);
    }

    void ShowScrollBar(int nBarType, BOOL bVisible = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, EM_SHOWSCROLLBAR, nBarType, bVisible);
    }
#endif // (_RICHEDIT_VER >= 0x0200)

#if (_RICHEDIT_VER >= 0x0300)
    BOOL SetTabStops(int nTabStops, LPINT rgTabStops)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_SETTABSTOPS, nTabStops, (LPARAM)rgTabStops);
    }

    BOOL SetTabStops()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_SETTABSTOPS, 0, 0L);
    }

    BOOL SetTabStops(const int& cxEachStop)    // takes an 'int'
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return (BOOL)::SendMessage(m_hWnd, EM_SETTABSTOPS, 1, (LPARAM)(LPINT)&cxEachStop);
    }
#endif // (_RICHEDIT_VER >= 0x0300)
};

typedef CRichEditCtrlT<ATL::CWindow>   CRichEditCtrl;


///////////////////////////////////////////////////////////////////////////////
// CRichEditCommands - message handlers for standard EDIT commands

// Chain to CRichEditCommands message map. Your class must also derive from CRichEditCtrl.
// Example:
// class CMyRichEdit : public CWindowImpl<CMyRichEdit, CRichEditCtrl>,
//                     public CRichEditCommands<CMyRichEdit>
// {
// public:
//      BEGIN_MSG_MAP(CMyRichEdit)
//              // your handlers...
//              CHAIN_MSG_MAP_ALT(CRichEditCommands<CMyRichEdit>, 1)
//      END_MSG_MAP()
//      // other stuff...
// };

template <class T>
class CRichEditCommands : public CEditCommands< T >
{
public:
    BEGIN_MSG_MAP(CRichEditCommands< T >)
        ALT_MSG_MAP(1)
        COMMAND_ID_HANDLER(ID_EDIT_CLEAR, CEditCommands< T >::OnEditClear)
        COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, CEditCommands< T >::OnEditClearAll)
        COMMAND_ID_HANDLER(ID_EDIT_COPY, CEditCommands< T >::OnEditCopy)
        COMMAND_ID_HANDLER(ID_EDIT_CUT, CEditCommands< T >::OnEditCut)
        COMMAND_ID_HANDLER(ID_EDIT_PASTE, CEditCommands< T >::OnEditPaste)
        COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, CEditCommands< T >::OnEditSelectAll)
        COMMAND_ID_HANDLER(ID_EDIT_UNDO, CEditCommands< T >::OnEditUndo)
#if (_RICHEDIT_VER >= 0x0200)
        COMMAND_ID_HANDLER(ID_EDIT_REDO, OnEditRedo)
#endif // (_RICHEDIT_VER >= 0x0200)
    END_MSG_MAP()

#if (_RICHEDIT_VER >= 0x0200)
    LRESULT OnEditRedo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        T* pT = static_cast<T*>(this);
        pT->Redo();
        return 0;
    }
#endif // (_RICHEDIT_VER >= 0x0200)

    // State (update UI) helpers
    BOOL CanCut() const
    { return HasSelection(); }

    BOOL CanCopy() const
    { return HasSelection(); }

    BOOL CanClear() const
    { return HasSelection(); }

    // Implementation
    BOOL HasSelection() const
    {
        const T* pT = static_cast<const T*>(this);
        return (pT->GetSelectionType() != SEL_EMPTY);
    }
};

#endif //__view_framework_atl_richedit_h__