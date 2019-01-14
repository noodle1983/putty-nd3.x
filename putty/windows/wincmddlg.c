/*
 * windlg.c - dialogs for PuTTY(tel), including the configuration dialog.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>

#include "putty.h"
#include "ssh.h"
#include "win_res.h"
#include "storage.h"
#include "dialog.h"

#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

#include <Shlobj.h>
#include "native_putty_common.h"
#include <vector>
#include <list>
#include <set>
#include <algorithm>  
using namespace std;
struct winctrl *dlg_findbyctrl(struct dlgparam *dp, union control *ctrl);

/*
 * These are the various bits of data required to handle the
 * portable-dialog stuff in the config box. Having them at file
 * scope in here isn't too bad a place to put them; if we were ever
 * to need more than one config box per process we could always
 * shift them to a per-config-box structure stored in GWL_USERDATA.
 */
static struct controlbox *ctrlbox;
/*
 * ctrls_base holds the OK and Cancel buttons: the controls which
 * are present in all dialog panels. ctrls_panel holds the ones
 * which change from panel to panel.
 */
// multi-thread is not supported
static struct winctrls ctrls_base;
static struct dlgparam dp;

static controlset* middle_btn_controlset = NULL;
static controlset* bottom_btn_controlset = NULL;


enum {
	IDCX_CMD = IDC_CMD,
    IDCX_TVSTATIC,
    IDCX_SEARCHBAR,
    IDCX_SESSIONTREEVIEW,
    IDCX_CLOUDTREEVIEW,
	IDCX_PROGRESS_STATIC,
	IDCX_PROGRESS_BAR,
	IDCX_PROGRESS_CANCEL_BTN,
    IDCX_STDBASE,
    IDCX_PANELBASE = IDCX_STDBASE + 32
};

static RECT dlgMonitorRect;

RECT getMaxWorkArea();

static void SaneEndDialog(HWND hwnd, int ret)
{
    SetWindowLongPtr(hwnd, BOXRESULT, ret);
    SetWindowLongPtr(hwnd, BOXFLAGS, DF_END);
}

void process_in_ui_jobs();
void ErrorExit(char * str) ;
static int SaneDialogBox(HINSTANCE hinst,
			 LPCTSTR tmpl,
			 HWND hwndparent,
			 DLGPROC lpDialogFunc)
{
    WNDCLASS wc;
    HWND hwnd;
    MSG msg;
    int flags;
    int ret;
    int gm;

    wc.style = CS_DBLCLKS | CS_SAVEBITS | CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc = DefDlgProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = DLGWINDOWEXTRA + 2*sizeof(LONG_PTR);
    wc.hInstance = hinst;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND +1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "PuTTY-ND2_Cmd";
    RegisterClassA(&wc);

    hwnd = CreateDialog(hinst, tmpl, hwndparent, lpDialogFunc);
	if (hwnd == NULL){
		ErrorExit("PuTTY-ND2_Cmd");
		return -1;
	}
	extern HWND hCmdWnd;
	hCmdWnd = hwnd;

	ShowWindow(hwnd, SW_HIDE);
	ShowWindow(hwnd, SW_SHOW);

	bringToForeground(hwnd);
    SetWindowLongPtr(hwnd, BOXFLAGS, 0); /* flags */
    SetWindowLongPtr(hwnd, BOXRESULT, 0); /* result from SaneEndDialog */

    while ((gm=GetMessage(&msg, NULL, 0, 0)) > 0) {
		process_in_ui_jobs();
    		
    	flags=GetWindowLongPtr(hwnd, BOXFLAGS);
    	if (!(flags & DF_END) && !IsDialogMessage(hwnd, &msg))
    	    DispatchMessage(&msg);
    	if (flags & DF_END)
    	    break;
    }

    if (gm == 0)
        PostQuitMessage(msg.wParam); /* We got a WM_QUIT, pass it on */

	HMONITOR mon;
	MONITORINFO mi;
	mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(mon, &mi);
	dlgMonitorRect = mi.rcWork;

    ret=GetWindowLongPtr(hwnd, BOXRESULT);
    DestroyWindow(hwnd);
	hCmdWnd = NULL;
	SetActiveWindow(hwndparent);
    return ret;
}

static void create_controls(HWND hwnd)
{
	char path[128] = { 0 };
	struct ctlpos cp;
	int index;
	int base_id;
	struct winctrls *wc;

	ctlposinit(&cp, hwnd, 3, 3, 16);
	wc = &ctrls_base;
	base_id = IDCX_STDBASE;

	for (index = -1; (index = ctrl_find_path(ctrlbox, path, index)) >= 0;) {
		struct controlset *s = ctrlbox->ctrlsets[index];
		winctrl_layout(&dp, wc, &cp, s, &base_id);
	}
}

/*
 * This function is the configuration box.
 * (Being a dialog procedure, in general it returns 0 if the default
 * dialog processing should be performed, and 1 if it should not.)
 */
static int CALLBACK GenericMainDlgProc(HWND hwnd, UINT msg,
				       WPARAM wParam, LPARAM lParam)
{
    HWND hw;
    int ret;

	if (msg == WM_INITDIALOG){
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		dp.hwnd = hwnd;
		{
			RECT rs, rd;
			rs = getMaxWorkArea();
			if (GetWindowRect(hwnd, &rd)){
				MapDialogRect(hwnd, &rd);
				MoveWindow(hwnd,
					(rs.right + rd.left - rd.right),
					(rs.bottom + rs.top + rd.top - rd.bottom) / 2,
					rd.right - rd.left, rd.bottom - rd.top, TRUE);
			}
		}
		create_controls(hwnd);     /* buttons etc */
		SetWindowText(hwnd, dp.wintitle);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG,
			(LPARAM)LoadIcon(hinst, MAKEINTRESOURCE(IDI_CFGICON)));

		SetWindowLongPtr(hwnd, GWLP_USERDATA, 1);
		return 0;
	}
	else if (msg == WM_NOTIFY){
		return winctrl_handle_command(&dp, msg, wParam, lParam);
	}
	else if (msg == WM_CLOSE){
			SaneEndDialog(hwnd, 0);
			return 0;
	}
	else if (msg == WM_SIZE){
		if (wParam == SIZE_MAXIMIZED)
			force_normal(hwnd);
		return 0;
	}
	else {
		/*
		 * Only process WM_COMMAND once the dialog is fully formed.
		 */
		if (GetWindowLongPtr(hwnd, GWLP_USERDATA) == 1) {
			ret = winctrl_handle_command(&dp, msg, wParam, lParam);
			if (dp.ended && GetCapture() != hwnd)
				SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
		}
		else
			ret = 0;
		return ret;
	}
    return 0;
}

static void on_button_callback(union control *ctrl, void *dlg,
	void *data, int event)
{
	if (event != EVENT_ACTION) { return; }
}

void setup_cmd_box(struct controlbox *b)
{
	struct controlset *s;
	union control *c, *bc;
	char *str;
	int i;

	s = ctrl_getset(b, "", "", "");
	middle_btn_controlset = s;
	ctrl_columns(s, 3, 43, 14, 43);
	c = ctrl_separator(s, I(10));
	c->generic.column = 1;
	c = ctrl_pushbutton(s,"all >>>", '\0',
		HELPCTX(no_help),
		on_button_callback, P(NULL));
	c->generic.column = 1;
	c = ctrl_separator(s, I(10));
	c->generic.column = 1;
	c = ctrl_pushbutton(s,"selected >", '\0',
		HELPCTX(no_help),
		on_button_callback, P(NULL));
	c->generic.column = 1;
	c = ctrl_separator(s, I(10));
	c->generic.column = 1;
	c = ctrl_pushbutton(s,"<<< all", '\0',
		HELPCTX(no_help),
		on_button_callback, P(NULL));
	c->generic.column = 1;
	c = ctrl_separator(s, I(10));
	c->generic.column = 1;
	c = ctrl_pushbutton(s,"< selected", '\0',
		HELPCTX(no_help),
		on_button_callback, P(NULL));
	c->generic.column = 1;
	c = ctrl_separator(s, I(10));
	c->generic.column = 1;
	c = ctrl_pushbutton(s, "apply", '\0',
		HELPCTX(no_help),
		on_button_callback, P(NULL));
	c->generic.column = 1;
	c = ctrl_separator(s, I(10));
	c->generic.column = 1;
	c = ctrl_pushbutton(s, "reset", '\0',
		HELPCTX(no_help),
		on_button_callback, P(NULL));
	c->generic.column = 1;
	c = ctrl_separator(s, I(108));
	c->generic.column = 1;
	
	s = ctrl_getset(b, "", "~bottom", "");
	bottom_btn_controlset = s;
	ctrl_columns(s, 7, 14, 14, 14, 15, 14, 15, 14);
	c = ctrl_pushbutton(s, "new folder", '\0',
		HELPCTX(no_help),
		on_button_callback, P(NULL));
	c->generic.column = 4;
	c = ctrl_pushbutton(s, "select none", '\0',
		HELPCTX(no_help),
		on_button_callback, P(NULL));
	c->generic.column = 5;
	c = ctrl_pushbutton(s, "delete", '\0',
		HELPCTX(no_help),
		on_button_callback, P(NULL));
	c->generic.column = 6;

}

int show_cmd_dlg(void)
{
    int ret;

	if (hCmdWnd){
		bringToForeground(hCmdWnd);
		return 0;
	}

    ctrlbox = ctrl_new_box();
    setup_cmd_box(ctrlbox);
    dp_init(&dp);
    winctrl_init(&ctrls_base);
    dp_add_tree(&dp, &ctrls_base);
    dp.wintitle = dupprintf("%s Cmd", appname);
    dp.errtitle = dupprintf("%s Error", appname);
	extern Conf* cfg;
    dp.data = cfg;
    dlg_auto_set_fixed_pitch_flag(&dp);
    dp.shortcuts['g'] = TRUE;	       /* the treeview: `Cate&gory' */

    ret = SaneDialogBox(hinst, MAKEINTRESOURCE(IDD_CMD_DLG_BOX), NULL,
		  GenericMainDlgProc);
	
    ctrl_free_box(ctrlbox);
	ctrlbox = NULL;
    winctrl_cleanup(&ctrls_base);
    dp_cleanup(&dp);
	
    return ret;
}
