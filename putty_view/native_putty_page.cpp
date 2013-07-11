
#include "native_putty_controller.h"
#include "native_putty_page.h"

#include "terminal.h"
#include "storage.h"

//-----------------------------------------------------------------------
//page related
//-----------------------------------------------------------------------
const LPCWSTR NativePuttyPage::WINTAB_PAGE_CLASS = L"WintabPage";

int NativePuttyPage::init(NativePuttyController* puttyController, const Config *cfg, HWND hwndParent)
{
	puttyController_ = puttyController;
    NativePuttyPage::registerPage();

    int winmode = /*WS_CHILD | */WS_VSCROLL | WS_POPUP|WS_VISIBLE|WS_SYSMENU;
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
    win_bind_data(hwndCtrl, this); 
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
        page_height, false);

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

//-----------------------------------------------------------------------
LRESULT CALLBACK NativePuttyPage::WndProc(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    //extern wintab tab;
    //debug(("[WintabpageWndProc]%s:%s\n", hwnd == tab.items[tab.cur]->page.hwndCtrl ? "PageMsg"
    //                        : "UnknowMsg", TranslateWMessage(message)));
    NativePuttyPage* page = (NativePuttyPage*)win_get_data(hwnd);
    if (page == NULL){
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
	NativePuttyController* puttyController = page->puttyController_;
    
    switch (message) {
		case WM_SETFOCUS:
			puttyController->onSetFocus();
			break;
		case WM_NETEVENT:
			puttyController->on_net_event(hwnd, message, wParam, lParam);
			break;
        case WM_COMMAND:
            puttyController->on_menu(hwnd, message, wParam, lParam);
            break;
        
        case WM_VSCROLL:
	        puttyController->on_scroll(hwnd, message, wParam, lParam);
        	break;
        case WM_NCPAINT:
        case WM_PAINT:
            puttyController->on_paint(hwnd, message,wParam, lParam);
    		break;

        //mouse button
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
         //   puttyController->process_log_status();
	        //puttyController->on_button(hwnd, message, wParam, lParam);
            return 0;
        case WM_MOUSEMOVE:
       /*     puttyController->on_mouse_move( hwnd, message, wParam, lParam);*/
        	return 0;
        case WM_NCMOUSEMOVE:
       /* 	puttyController->on_nc_mouse_move( hwnd, message, wParam, lParam);*/
        	break;

        //paste       
        case WM_GOT_CLIPDATA:
        	/*if (process_clipdata((HGLOBAL)lParam, wParam))
    	        term_do_paste(puttyController->term);*/
        	return 0;
        case WM_IGNORE_CLIP:
        	/*puttyController->ignore_clip = wParam;	*/       /* don't panic on DESTROYCLIPBOARD */
        	break;
        case WM_DESTROYCLIPBOARD:
        	/*if (!puttyController->ignore_clip)
        	    term_deselect(puttyController->term);
        	puttyController->ignore_clip = FALSE;*/
        	return 0;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            /*if (puttyController->swallow_shortcut_key(message, wParam, lParam))
                break;
	        if (puttyController->on_key( hwnd, message,wParam, lParam))
                break;*/
			return 0;
        default:
            break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
