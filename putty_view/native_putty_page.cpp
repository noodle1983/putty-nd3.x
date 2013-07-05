#include "native_putty_page.h"

#include "terminal.h"
#include "storage.h"

//-----------------------------------------------------------------------
//page related
//-----------------------------------------------------------------------
const LPCWSTR NativePuttyPage::WINTAB_PAGE_CLASS = L"WintabPage";

int NativePuttyPage::init(const Config *cfg, HWND hwndParent)
{
    NativePuttyPage::registerPage();

    int winmode = WS_CHILD | WS_VSCROLL ;
	if (!cfg->scrollbar)
	    winmode &= ~(WS_VSCROLL);
	hwndCtrl = CreateWindowEx(
                    WS_EX_TOPMOST, 
                    WINTAB_PAGE_CLASS, 
                    WINTAB_PAGE_CLASS,
			        winmode, 
			        0, 0, 0, 0,
			        hwndParent, 
			        NULL,   /* hMenu */
			        hinst, 
			        NULL);  /* lpParam */

    if (hwndCtrl == NULL){
        ErrorExit("CreatePage");
        ExitProcess(2); 
    }
    
    return 0;

}

//-----------------------------------------------------------------------

void NativePuttyPage::init_scrollbar(Terminal *term)
{
    SCROLLINFO si;

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = term->rows - 1;
	si.nPage = term->rows;
	si.nPos = 0;
	SetScrollInfo(hwndCtrl, SB_VERT, &si, FALSE);
}

//-----------------------------------------------------------------------

int NativePuttyPage::fini()
{
    DestroyWindow(hwndCtrl);
    return 0;
}

//-----------------------------------------------------------------------

int NativePuttyPage::resize(const RECT *rc, const int cfg_winborder)
{
    RECT pc;
    int page_width = rc->right - rc->left;
    int page_height = rc->bottom - rc->top;
    MoveWindow(hwndCtrl, rc->left, rc->top, 
        page_width, 
        page_height, TRUE);

    GetClientRect(hwndCtrl, &pc);
    extra_page_width = page_width - (pc.right - pc.left) + cfg_winborder*2;
    extra_page_height = page_height - (pc.bottom - pc.top) + cfg_winborder*2;
    return 0;
}

//-----------------------------------------------------------------------


/*
 * not thread safe
 */
int NativePuttyPage::registerPage()
{
    /*
    WNDCLASS wndclass;
    wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;;
	wndclass.lpfnWndProc = WintabpageWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hinst;
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_IBEAM);
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = WINTAB_PAGE_CLASS;
	RegisterClass(&wndclass))
        ErrorExit();
        */

    WNDCLASSEX wndclass;
	wndclass.cbSize = sizeof(wndclass);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hinst;
	wndclass.hIcon = NULL;
	wndclass.hCursor = NULL;
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = WINTAB_PAGE_CLASS;
	wndclass.hIconSm = 0;
	RegisterClassEx(&wndclass);
    return 0;
}

//-----------------------------------------------------------------------


int NativePuttyPage::unregisterPage()
{
    return UnregisterClass(WINTAB_PAGE_CLASS, hinst);
}