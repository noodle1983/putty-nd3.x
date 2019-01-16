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
extern SavedCmd g_saved_cmd;

static char pre_cmd[256] = { 0 };
static HWND hEdit = NULL;
enum {
	EDIT_BEGIN = 0,
	EDIT_END = 1,
	EDIT_OK = 2,
	EDIT_CANCEL = 3,
	EDIT_INIT = 4
};

enum {
	CMD_GROUP = 0,
	CMD_ITEM = 1,
	CMD_NONE = 2,
	CMD_DEFAULT = 3
};

enum {
	IDCX_CMD = IDC_CMD,
    IDCX_TVSTATIC,
    IDCX_SEARCHBAR,
    IDCX_CMDTREEVIEW,
    IDCX_STDBASE,
    IDCX_PANELBASE = IDCX_STDBASE + 32
};
struct treeview_faff {
	HWND treeview;
	HTREEITEM lastat[128];
};

static const int CMD_TREEVIEW_WIDTH = 80;
static const int TREEVIEW_HEIGHT = 225;
static RECT dlgMonitorRect;

static void refresh_cmd_treeview(const char* select_cmd);
static int edit_cmd_treeview(HWND hwndCmd, int eflag);
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
		if (msg.message == WM_KEYUP) {
		}
		else if (msg.message == WM_KEYDOWN) {
			if (msg.wParam == VK_CONTROL) {
			}
			else if (msg.wParam == VK_F2)
			{
				HWND hwndSess = GetDlgItem(hwnd, IDCX_CMDTREEVIEW);
				TreeView_EditLabel(hwndSess, TreeView_GetSelection(hwndSess));
				continue;
			}
			if (msg.wParam == VK_RETURN) {
				if (edit_cmd_treeview(GetDlgItem(hwnd, IDCX_CMDTREEVIEW), EDIT_OK)) {
					continue;
				}
			}
			if (msg.wParam == VK_ESCAPE) {
				if (edit_cmd_treeview(GetDlgItem(hwnd, IDCX_CMDTREEVIEW), EDIT_CANCEL)) {
					continue;

				}
				
			}
		}
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

/*
* Create the cmd tree view.
*/
static HWND create_cmd_treeview(HWND hwnd)
{
	RECT r;
	WPARAM font;
	HWND tvstatic;
	HWND cmdview;
	HIMAGELIST hImageList;
	HBITMAP hBitMap;

	r.left = 3;
	r.right = r.left + CMD_TREEVIEW_WIDTH - 6;
	r.top = 3;
	r.bottom = r.top + 10;
	MapDialogRect(hwnd, &r);
	const int SEARCH_TEXT_LEN = CMD_TREEVIEW_WIDTH;
	tvstatic = CreateWindowEx(0, "STATIC", "Saved Commands:",
		WS_CHILD | WS_VISIBLE,
		r.left, r.top,
		SEARCH_TEXT_LEN * 2, r.bottom - r.top,
		hwnd, (HMENU)IDCX_TVSTATIC, hinst,
		NULL);
	font = SendMessage(hwnd, WM_GETFONT, 0, 0);
	SendMessage(tvstatic, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

	r.left = 3;
	r.right = r.left + CMD_TREEVIEW_WIDTH - 6;
	r.top = 13;
	r.bottom = r.top + TREEVIEW_HEIGHT;
	MapDialogRect(hwnd, &r);
	cmdview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
		WS_CHILD | WS_VISIBLE |
		WS_TABSTOP | TVS_HASLINES |
		TVS_HASBUTTONS | TVS_EDITLABELS |//TVS_LINESATROOT |
		TVS_SHOWSELALWAYS, r.left, r.top,
		r.right - r.left, r.bottom - r.top,
		hwnd, (HMENU)IDCX_CMDTREEVIEW, hinst,
		NULL);
	font = SendMessage(hwnd, WM_GETFONT, 0, 0);
	SendMessage(cmdview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

	return cmdview;
}

extern void get_cmdlist(vector<string> & cmdlist);
static HTREEITEM cmd_treeview_insert(HWND hCmdWnd, HTREEITEM preitem, char *text, char *path)
{
	TVINSERTSTRUCT ins;
	int i;
	HTREEITEM newitem;
	ins.hParent = TVI_ROOT;
	ins.hInsertAfter = preitem;
#if _WIN32_IE >= 0x0400 && defined NONAMELESSUNION
#define INSITEM DUMMYUNIONNAME.item
#else
#define INSITEM item
#endif
	ins.INSITEM.mask = TVIF_TEXT | TVIF_PARAM;
	ins.INSITEM.pszText = text;
	ins.INSITEM.cchTextMax = strlen(text) + 1;
	ins.INSITEM.lParam = CMD_ITEM;
	newitem = TreeView_InsertItem(hCmdWnd, &ins);
	return newitem;
}

static void refresh_cmd_treeview(const char* select_cmd)
{
	HWND cmdview = GetDlgItem(dp.hwnd, IDCX_CMDTREEVIEW);

	HTREEITEM hfirst = NULL;
	HTREEITEM item = NULL;
	int i, j, k;               //index to iterator all the characters of the cmds
	int level;              //tree item's level
	int b;                  //index of the tree item's first character
	char itemstr[64];
	char selected_cmd_name[256] = { 0 };
	char lower_cmd_name[256] = { 0 };
	char pre_show_cmd_name[256] = { 0 };
	vector<string> cmdlist;
	int is_select;
	char cmd[256] = { 0 };

	TreeView_DeleteAllItems(cmdview);

	get_cmdlist(cmdlist);

	std::vector<string>::iterator it = cmdlist.begin();
	for (; it != cmdlist.end(); it++) {
		string& cmd_name = *it;

		is_select = !strcmp(cmd_name.c_str(), select_cmd);
		
		item = cmd_treeview_insert(cmdview, item, const_cast<char*>(cmd_name.c_str()), const_cast<char*>(cmd_name.c_str()));
		if (is_select) {
			hfirst = item;
			strncpy(selected_cmd_name, cmd_name.c_str(), sizeof(selected_cmd_name));
		}

		if (!hfirst) {
			hfirst = item;
			strncpy(selected_cmd_name, cmd_name.c_str(), sizeof(selected_cmd_name));
		}
	}
	InvalidateRect(cmdview, NULL, TRUE);
	if (hfirst) {
		TreeView_SelectItem(cmdview, hfirst);
	}
	else {
		dlg_refresh(NULL, &dp);
	}
}

/*
* convert treeview to session
*/
LPARAM conv_tv_to_cmd(
	HWND hwndSess, HTREEITEM hitem,
	char* const cmd_name, const int name_len)
{
	TVITEM item;
	HTREEITEM i = hitem;
	char buffer[256];
	int item_len = 0;
	int left = name_len - 1;

	memset(cmd_name, 0, name_len);
	item.hItem = i;
	item.pszText = buffer;
	item.cchTextMax = sizeof(buffer);
	item.mask = TVIF_PARAM | TVIF_TEXT;
	TreeView_GetItem(hwndSess, &item);
	item_len = strlen(buffer);
	//no enough space, return empty session name
	if (item_len > left) {
		return CMD_NONE;
	}

	memcpy(cmd_name, buffer, item_len);
	return CMD_ITEM;
}

/*
* get selected session name configuration.
*/
LPARAM get_selected_cmd(HWND hwndSess, char* const cmd_name, const int name_len)
{

	HTREEITEM hitem =
		TreeView_GetSelection(hwndSess);
	return conv_tv_to_cmd(hwndSess, hitem, cmd_name, name_len);
}

/*
* handle edit message for cmd treeview.
* return if the message belongs to the cmd treeview edit control
*/
static int edit_cmd_treeview(HWND hwndSess, int eflag)
{
	char buffer[256] = { 0 };

	char itemstr[64] = { 0 };
	char to_cmd[256] = { 0 };
	int i = 0;
	int pos = 0;
	char* c = NULL;
	TVITEM item;
	HTREEITEM hi;
	int cmd_flags = CMD_NONE;

	if (!hwndSess)
		return FALSE;

	switch (eflag) {
	case EDIT_INIT:
		hEdit = NULL;
		return TRUE;
		break;
	case EDIT_BEGIN:
		/* get the pre_session */
	
		cmd_flags = get_selected_cmd(hwndSess, pre_cmd, sizeof pre_cmd);
		if (is_pre_defined_cmd(pre_cmd) || cmd_flags == CMD_NONE) {
			hEdit = NULL;
			TreeView_EndEditLabelNow(hwndSess, TRUE);
			return TRUE;
		}
		hEdit = TreeView_GetEditControl(hwndSess);
		return TRUE;
		break;

	case EDIT_CANCEL:
		if (hEdit == NULL || pre_cmd[0] == '\0')
			return FALSE;
		hEdit = NULL;
		TreeView_EndEditLabelNow(hwndSess, TRUE);
		return TRUE;
		break;

	case EDIT_OK:
		if (hEdit == NULL || pre_cmd[0] == '\0')
			return FALSE;
		TreeView_EndEditLabelNow(hwndSess, FALSE);
		return TRUE;
		break;

	case EDIT_END:
		if (hEdit == NULL || pre_cmd[0] == '\0')
			return FALSE;

		GetWindowText(hEdit, buffer, sizeof(buffer));

		/*validate the buffer*/
		if (buffer[0] == '\0')
			return TRUE;
		for (i = 0; i < strlen(buffer); i++) {
			if (buffer[i] == '#' || buffer[i] == '/' || buffer[i] == '\\')
				buffer[i] = '%';
		}
		buffer[i] = '\0';

		/* if no changed, return */
		hi = TreeView_GetSelection(hwndSess);
		item.hItem = hi;
		item.pszText = itemstr;
		item.cchTextMax = sizeof(itemstr);
		item.mask = TVIF_TEXT | TVIF_PARAM;
		TreeView_GetItem(hwndSess, &item);
		if (!strcmp(item.pszText, buffer)) {
			hEdit = NULL;
			return TRUE;
		}
		cmd_flags = item.lParam;

		/* calc the to_session */
		strncpy(to_cmd, buffer, sizeof(to_cmd)-1);

		/* check if to session exists */
		vector<string> cmdlist;
		get_cmdlist(cmdlist);
		extern bool cmdstdcmp(const std::string& av, const std::string& bv);
		vector<string>::iterator it = lower_bound(cmdlist.begin(), cmdlist.end(), to_cmd, cmdstdcmp);		
		if (it != cmdlist.end() && strcmp(it->c_str(), to_cmd) == 0 ) {
			/* to_session exists */
			MessageBox(GetParent(hwndSess), "Destination scripts is already exist.", "Error", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
			hEdit = NULL;
			return TRUE;
		}

		/* now rename sessions */
		move_cmd_settings(pre_cmd, to_cmd);		
		strncpy(pre_cmd, to_cmd, 256);

		/* clean */
		hEdit = NULL;

		refresh_cmd_treeview(to_cmd);

		break;
		return TRUE;
	}
	return FALSE;
}

/*
* delete session item/group
*/
static void del_cmd_treeview()
{
	HWND cmdview = GetDlgItem(dp.hwnd, IDCX_CMDTREEVIEW);
	char cmd_name[512] = { 0 };
	HTREEITEM hitem = TreeView_GetSelection(cmdview);
	conv_tv_to_cmd(cmdview, hitem, cmd_name, sizeof(cmd_name));
	if (strlen(cmd_name) == 0) { return; }

	if (!strcmp(cmd_name, TMP_CMD_NAME)) {
		gStorage->del_cmd_settings(cmd_name);
		strncpy(pre_cmd, TMP_CMD_NAME, sizeof(pre_cmd));
		load_cmd_settings(TMP_CMD_NAME, g_saved_cmd);
		dlg_refresh(NULL, &dp);
		return;
	}

	gStorage->del_cmd_settings(cmd_name);
	pre_cmd[0] = '\0';

	TreeView_DeleteItem(cmdview, hitem);
	return;
}

static LPARAM change_selected_cmd(HWND hwndCmd)
{
	char cmd_name[256];
	LPARAM selected_flags;

	selected_flags = get_selected_cmd(hwndCmd, cmd_name, 256);
	if (cmd_name[0] == '\0') { strcpy(cmd_name, TMP_CMD_NAME); }

	if (!strcmp(cmd_name, pre_cmd)) { return selected_flags; }

	save_cmd_settings(pre_cmd, g_saved_cmd);

	strncpy(pre_cmd, cmd_name, 256);
	load_cmd_settings(cmd_name, g_saved_cmd);

	dlg_refresh(NULL, &dp);

	return selected_flags;
}

static void create_controls(HWND hwnd, char* _path)
{
	char path[128] = { 0 };
	struct ctlpos cp;
	int index;
	int base_id;
	struct winctrls *wc;

	strncpy(path, _path, sizeof(path) - 1);
	if (!path[0]) {
		ctlposinit(&cp, hwnd, 0, 0, TREEVIEW_HEIGHT + 16);
		base_id = IDCX_STDBASE;
	}
	else {
		ctlposinit(&cp, hwnd, CMD_TREEVIEW_WIDTH, 0, 13);
		base_id = IDCX_PANELBASE;
	}
	wc = &ctrls_base;

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
    HWND hw, cmdview;
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
		create_controls(hwnd, "");     /* buttons etc */
		create_controls(hwnd, "Command");     /* buttons etc */
		SetWindowText(hwnd, dp.wintitle);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG,
			(LPARAM)LoadIcon(hinst, MAKEINTRESOURCE(IDI_MAINICON)));

		cmdview = create_cmd_treeview(hwnd);
		refresh_cmd_treeview(TMP_CMD_NAME);

		SetWindowLongPtr(hwnd, GWLP_USERDATA, 1);
		return 0;
	}
	else if (msg == WM_NOTIFY){
		if (LOWORD(wParam) == IDCX_CMDTREEVIEW) {
			switch (((LPNMHDR)lParam)->code) {
			case TVN_SELCHANGED:
				change_selected_cmd(((LPNMHDR)lParam)->hwndFrom);
				break;

			case NM_DBLCLK:
				break;
			case TVN_KEYDOWN:
				break;

			case TVN_BEGINLABELEDIT:
				edit_cmd_treeview(((LPNMHDR)lParam)->hwndFrom, EDIT_BEGIN);
				break;

			case TVN_ENDLABELEDIT:
				edit_cmd_treeview(((LPNMHDR)lParam)->hwndFrom, EDIT_END);
				break;

			case NM_RCLICK:
				break;

			case TVN_BEGINDRAG:
				break;

			case TVN_ITEMEXPANDED:
				break;
			default:
				break;

			};//switch
			return 0;
		}
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

static void on_button_save_cmd(union control *ctrl, void *dlg,
	void *data, int event)
{
	if (event != EVENT_ACTION) { return; }

	union control * name_ctrl = (union control *)(ctrl->generic.context.p);
	char *name = dlg_editbox_get(name_ctrl, dlg);
	save_cmd_settings(name, g_saved_cmd);
	refresh_cmd_treeview(name);
	sfree(name);
}

static void on_button_add_cmd(union control *ctrl, void *dlg,
	void *data, int event)
{
	if (event != EVENT_ACTION) { return; }

	/* check if to session exists */
	vector<string> cmdlist;
	get_cmdlist(cmdlist);
	extern bool cmdstdcmp(const std::string& av, const std::string& bv);

	char new_cmd_name[512] = { 0 };
	int i = cmdlist.size() + 1;

	while (true)
	{
		snprintf(new_cmd_name, sizeof(new_cmd_name), "Command %d", i);

		vector<string>::iterator it = lower_bound(cmdlist.begin(), cmdlist.end(), new_cmd_name, cmdstdcmp);
		if (it == cmdlist.end() || strcmp(it->c_str(), new_cmd_name) != 0) {
			break;
		}
		i++;
	}
	
	SavedCmd cmd;
	save_cmd_settings(new_cmd_name, cmd);
	refresh_cmd_treeview(new_cmd_name);
	HWND cmdview = GetDlgItem(dp.hwnd, IDCX_CMDTREEVIEW);
	TreeView_EditLabel(cmdview, TreeView_GetSelection(cmdview));
}

static void on_button_del_cmd(union control *ctrl, void *dlg,
	void *data, int event)
{
	if (event != EVENT_ACTION) { return; }

	del_cmd_treeview();
}


void cmd_name_handler(union control *ctrl, void *dlg,
	void *data, int event)
{
	if (event == EVENT_REFRESH) {	
		dlg_editbox_set(ctrl, dlg, strcmp(pre_cmd, TMP_CMD_NAME) == 0 ? "" : pre_cmd);
	}
	else if (event == EVENT_VALCHANGE) {
		//char *field = dlg_editbox_get(ctrl, dlg);
		//sfree(field);
	}
}

void cmd_scripts_handler(union control *ctrl, void *dlg,
	void *data, int event)
{
	if (event == EVENT_REFRESH) {
		dlg_editbox_set(ctrl, dlg, g_saved_cmd.scripts.c_str());
	}
	else if (event == EVENT_VALCHANGE) {
		char *field = dlg_editbox_get(ctrl, dlg);
		g_saved_cmd.scripts = field;
		sfree(field);
	}
}

void cmd_param_handler(union control *ctrl, void *dlg,
	void *data, int event)
{
	if (event == EVENT_REFRESH) {
		dlg_editbox_set(ctrl, dlg, g_saved_cmd.replace.c_str());
	}
	else if (event == EVENT_VALCHANGE) {
		char *field = dlg_editbox_get(ctrl, dlg);
		g_saved_cmd.replace = field;
		sfree(field);
	}
}

void setup_cmd_box(struct controlbox *b)
{
	struct controlset *s;
	union control *c, *bc;
	char *str;
	int i;

	s = ctrl_getset(b, "", "", "");
	middle_btn_controlset = s;
	ctrl_columns(s, 5, 10, 10, 40, 20, 20);
	c = ctrl_pushbutton(s,"+", '\0',
		HELPCTX(no_help),
		on_button_add_cmd, P(NULL));
	c->generic.column = 0;
	c = ctrl_pushbutton(s, "-", '\0',
		HELPCTX(no_help),
		on_button_del_cmd, P(NULL));
	c->generic.column = 1;
	c = ctrl_pushbutton(s, "send", '\0',
		HELPCTX(no_help),
		on_button_callback, P(NULL));
	c->generic.column = 3;
	c = ctrl_pushbutton(s, "send to all tab", '\0',
		HELPCTX(no_help),
		on_button_callback, P(NULL));
	c->generic.column = 4;

	s = ctrl_getset(b, "Command", "", "");
	ctrl_columns(s, 2, 20, 80);
	c = ctrl_pushbutton(s, "save as:", '\0',
		HELPCTX(no_help),
		on_button_save_cmd, P(NULL));
	c->generic.column = 0;
	union control *save_button = c;
	c = ctrl_editbox(s, "", '\0', 99,
		HELPCTX(no_help),
		cmd_name_handler, I(0), I(0));
	c->generic.column = 1;
	save_button->generic.context = P(c);

	ctrl_columns(s, 1, 100);
	ctrl_editbox(s, "Scripts(Ctrl + Enter to send):", '\0' , 1600,
		HELPCTX(no_help),
		cmd_scripts_handler, I(0), I(0));

	s = ctrl_getset(b, "Command", "", "");
	ctrl_editbox(s, "Param:", '\0', 200,
		HELPCTX(no_help),
		cmd_param_handler, I(0), I(0));
	ctrl_text(s, "1.\"$a=1;$b=2\" will replace all the $a with 1 and all the $b with 2", HELPCTX(no_help));

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
    dp.wintitle = dupprintf("%s Command Dialog", appname);
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
