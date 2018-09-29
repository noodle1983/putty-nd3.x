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
struct winctrl *dlg_findbyctrl(struct dlgparam *dp, union control *ctrl);
#ifdef MSVC4
#define TVINSERTSTRUCT  TV_INSERTSTRUCT
#define TVITEM          TV_ITEM
#define ICON_BIG        1
#endif

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
static struct winctrls ctrls_base, ctrls_panel;
static struct dlgparam dp;


static char pre_session[256] = {0};
static int dragging = FALSE;
static bool isFreshingSessionTreeView = false;
static bool isFreshingTreeView = false;
static HMENU st_popup_menus[4];
enum {
    SESSION_GROUP = 0 ,
    SESSION_ITEM  = 1,
	SESSION_NONE  = 2,
	SESSION_DEFAULT  = 3
};

enum {
    DRAG_BEGIN = 0 ,
    DRAG_MOVE  = 1,
	DRAG_CTRL_DOWN = 2,
	DRAG_CTRL_UP   = 3,
	DRAG_END   = 4,
	DRAG_CANCEL = 5
};

static HWND hEdit = NULL;
enum {
    EDIT_BEGIN = 0 ,
    EDIT_END  = 1,
	EDIT_OK = 2,
	EDIT_CANCEL   = 3,
	EDIT_INIT   = 4
};


enum {
    IDCX_ABOUT = IDC_ABOUT,
    IDCX_TVSTATIC,
    IDCX_SEARCHBAR,
    IDCX_SESSIONTREEVIEW,
    IDCX_TREEVIEW,
    IDCX_STDBASE,
    IDCX_PANELBASE = IDCX_STDBASE + 32
};

enum {
    /* 
     * please make sure the menu id is 
     * in the range (IDM_ST_NONE, IDM_ST_ROOF) 
     */
    IDM_ST_NONE    = 0 ,
    IDM_ST_NEWSESS = 1 ,
    IDM_ST_NEWGRP  = 2 ,
    IDM_ST_DUPSESS = 3 ,
    IDM_ST_DUPGRP  = 4 ,
    IDM_ST_NEWSESSONGRP  = 5 ,
    IDM_ST_DEL     = 6 ,
    IDM_ST_BACKUP  = 7 ,
    IDM_ST_BACKUP_ALL  = 8 ,
    IDM_ST_RESTORE = 9 ,
    IDM_ST_ROOF    = 10
};

struct treeview_faff {
    HWND treeview;
    HTREEITEM lastat[128];
};

const BYTE ANDmaskCursor[] = { 0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFE,0x7F, //line 0 - 3
							 0xFE,0x7F, 0xFE,0x7F, 0xFE,0x7F, 0xE0,0x07, //line 4 - 7 

							 0xE0,0x07, 0xFE,0x7F, 0xFE,0x7F, 0xFE,0x7F, //line 8 - 11 
							 0xFE,0x7F, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, //line 12 - 16 
							 }; 
const BYTE XORmaskCursor[] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

HCURSOR hCopyCurs = NULL; 
int showSessionTreeview = 0;
const int SESSION_TREEVIEW_WIDTH = 160;
const int TREEVIEW_HEIGHT = 269;
RECT dlgMonitorRect;

extern Conf* cfg;		       /* defined in window.c */

#define PRINTER_DISABLED_STRING "None (printing disabled)"

#define GRP_COLLAPSE_SETTING "GroupCollapse"

static void refresh_session_treeview(
    HWND sessionview, struct treeview_faff* tvfaff, 
    const char* select_session);
static int drag_session_treeview(
	HWND hwndSess, int flags, 
	WPARAM wParam, LPARAM lParam);
static int edit_session_treeview(HWND hwndSess, int eflag);
RECT getMaxWorkArea();
LPARAM get_selected_session(HWND hwndSess, char* const sess_name, const int name_len);

void force_normal(HWND hwnd)
{
    static int recurse = 0;

    WINDOWPLACEMENT wp;

    if (recurse)
	return;
    recurse = 1;

    wp.length = sizeof(wp);
    if (GetWindowPlacement(hwnd, &wp) && wp.showCmd == SW_SHOWMAXIMIZED) {
	wp.showCmd = SW_SHOWNORMAL;
	SetWindowPlacement(hwnd, &wp);
    }
    recurse = 0;
}



static int CALLBACK LicenceProc(HWND hwnd, UINT msg,
				WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
      case WM_INITDIALOG:
	{
	    char *str = dupprintf("%s Licence", appname);
	    SetWindowText(hwnd, str);
	    sfree(str);
	}
	return 1;
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
	  case IDOK:
	  case IDCANCEL:
	    EndDialog(hwnd, 1);
	    return 0;
	}
	return 0;
      case WM_CLOSE:
	EndDialog(hwnd, 1);
	return 0;
    }
    return 0;
}

static int CALLBACK AboutProc(HWND hwnd, UINT msg,
			      WPARAM wParam, LPARAM lParam)
{
    char *str;

    switch (msg) {
      case WM_INITDIALOG:
	str = dupprintf("About %s", appname);
	SetWindowText(hwnd, str);
	sfree(str);
	SetDlgItemText(hwnd, IDA_TEXT1, appname);
	SetDlgItemText(hwnd, IDA_VERSION, ver);
	return 1;
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
	  case IDOK:
	  case IDCANCEL:
	    EndDialog(hwnd, TRUE);
	    return 0;
	  case IDA_LICENCE:
	    EnableWindow(hwnd, 0);
	    DialogBox(hinst, MAKEINTRESOURCE(IDD_LICENCEBOX),
		      hwnd, LicenceProc);
	    EnableWindow(hwnd, 1);
	    SetActiveWindow(hwnd);
	    return 0;

	  case IDA_WEB:
	    /* Load web browser */
	    ShellExecute(hwnd, "open",
			 "http://sourceforge.net/projects/putty-nd/",
			 0, 0, SW_SHOWDEFAULT);
	    return 0;
	}
	return 0;
      case WM_CLOSE:
	EndDialog(hwnd, TRUE);
	return 0;
    }
    return 0;
}

char InputBoxStr[4096] = "\0";
char InputBoxCaption[64] = "\0";
char InputBoxTips[128] = "\0";
BOOL CALLBACK InputDialogProc(HWND hwndDlg,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowText(hwndDlg, InputBoxCaption);
		SetDlgItemText(hwndDlg, 100, InputBoxTips);
		SetDlgItemText(hwndDlg, 102, InputBoxStr);
		SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		extern HWND hInputWnd;
		hInputWnd = hwndDlg;
		/*
		* Centre the window.
		*/
		{			       /* centre the window */
			RECT rs, rd;

			//hw = GetDesktopWindow();
			//if (GetWindowRect(hw, &rs) && GetWindowRect(hwnd, &rd)){
			//rs = getMaxWorkArea();
			extern HWND get_top_win();
			GetWindowRect(get_top_win(), &rs);
			if (GetWindowRect(hwndDlg, &rd)){
				MoveWindow(hwndDlg,
					(rs.right + rs.left + rd.left - rd.right) / 2,
					(rs.bottom + rs.top + rd.top - rd.bottom) / 2,
					rd.right - rd.left, rd.bottom - rd.top, TRUE);
			}
		}
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (!GetDlgItemText(hwndDlg, 102, InputBoxStr, sizeof(InputBoxStr)))
				*InputBoxStr = 0;

			// Fall through. 

		case IDCANCEL:
			EndDialog(hwndDlg, wParam);
			extern HWND hInputWnd;
			hInputWnd = NULL;
			return TRUE;
		}
	}
	return FALSE;
}

static void SaneEndDialog(HWND hwnd, int ret)
{
    SetWindowLongPtr(hwnd, BOXRESULT, ret);
    SetWindowLongPtr(hwnd, BOXFLAGS, DF_END);
}

#include <map>
void got_adb_devices(std::map<std::string, std::string> & deviceMap)
{
	struct sesslist sesslist;
	get_sesslist(&sesslist, TRUE);
	int android_dir_folder_len = strlen(ANDROID_DIR_FOLDER_NAME);
	for (int i = 0; i < sesslist.nsessions; i++){
		if (!sesslist.sessions[i][0])
			continue;
		if (memcmp(ANDROID_DIR_FOLDER_NAME, sesslist.sessions[i], android_dir_folder_len) != 0)
			continue;
		char* deviceId = sesslist.sessions[i] + android_dir_folder_len;
		std::map<std::string, std::string>::iterator it = deviceMap.find(deviceId);
		if (it != deviceMap.end())
		{
			deviceMap.erase(it);
		}
	}

	for (std::map<std::string, std::string>::iterator it = deviceMap.begin(); it != deviceMap.end(); it++)
	{
		Conf* tmpCfg = conf_new();
		load_settings(ANDROID_DIR_FOLDER_NAME, tmpCfg);

		char new_session_name[512] = { 0 };
		strncpy(new_session_name, ANDROID_DIR_FOLDER_NAME, sizeof(new_session_name)-1);
		strncat(new_session_name, it->first.c_str(), sizeof(new_session_name)-1);
		conf_set_int(tmpCfg, CONF_protocol, PROT_ADB);
		conf_set_str(tmpCfg, CONF_adb_con_str, it->first.c_str());
		save_settings(new_session_name, tmpCfg);
		conf_free(tmpCfg);
	}

	get_sesslist(&sesslist, FALSE);
	
	if (!deviceMap.empty())
	{
		struct treeview_faff tvfaff;
		tvfaff.treeview = GetDlgItem(hConfigWnd, IDCX_SESSIONTREEVIEW);
		memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
		refresh_session_treeview(tvfaff.treeview, &tvfaff, ANDROID_DIR_FOLDER_NAME);
	}
}

void start_adb_scan();
void stop_adb_scan();
void check_update_device();
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
    wc.lpszClassName = "PuTTY-ND2_ConfigBox";
    RegisterClassA(&wc);

    hwnd = CreateDialog(hinst, tmpl, hwndparent, lpDialogFunc);
	if (hwnd == NULL){
		ErrorExit("PuTTY-ND2_ConfigBox");
		return -1;
	}
	extern HWND hConfigWnd;
	hConfigWnd = hwnd;

	ShowWindow(hwnd, SW_HIDE);
	ShowWindow(hwnd, SW_SHOW);

	bringToForeground(hwnd);
    SetWindowLongPtr(hwnd, BOXFLAGS, 0); /* flags */
    SetWindowLongPtr(hwnd, BOXRESULT, 0); /* result from SaneEndDialog */

    while ((gm=GetMessage(&msg, NULL, 0, 0)) > 0) {
    	if(msg.message == WM_KEYUP){
    		if (msg.wParam == VK_CONTROL)
    			drag_session_treeview(NULL
    				, DRAG_CTRL_UP, msg.wParam, msg.lParam);
    	}else if (msg.message == WM_KEYDOWN){
			if (msg.wParam == VK_CONTROL){
				drag_session_treeview(NULL
					, DRAG_CTRL_DOWN, msg.wParam, msg.lParam);
			}
            else if (msg.wParam == VK_RETURN){
                if ( edit_session_treeview(GetDlgItem(hwnd,IDCX_SESSIONTREEVIEW), EDIT_OK)){
                	continue;
				}
				else if (ctrlbox->okbutton != NULL){
					struct winctrl *wc = dlg_findbyctrl(&dp, ctrlbox->okbutton);
					winctrl_handle_command(&dp, 
						WM_COMMAND, 
						MAKEWPARAM(wc->base_id, BN_CLICKED), 
						0);
					if (dp.ended){
						SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
						break;
					}
					continue;
				}
            }
            else if (msg.wParam == VK_ESCAPE){
                if(edit_session_treeview(GetDlgItem(hwnd,IDCX_SESSIONTREEVIEW), EDIT_CANCEL)
                    || drag_session_treeview(GetDlgItem(hwnd,IDCX_SESSIONTREEVIEW), 
                                                DRAG_CANCEL, msg.wParam, msg.lParam)){
                	continue;

				}
				else if (ctrlbox->cancelbutton != NULL){
					struct winctrl *wc = dlg_findbyctrl(&dp, ctrlbox->cancelbutton);
					winctrl_handle_command(&dp, 
						WM_COMMAND, 
						MAKEWPARAM(wc->base_id, BN_CLICKED), 
						0);
					if (dp.ended){
						SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
						break;
					}
					continue;
				}
            }
			else if (load_global_isetting(SHORTCUT_KEY_RENAME_SESSION "Enable", 1) != 0)
			{
				int key_type = load_global_isetting(SHORTCUT_KEY_RENAME_SESSION "Type", F2);
				int key_val = load_global_isetting(SHORTCUT_KEY_RENAME_SESSION "Key", VK_TAB);
				BYTE keystate[256];
				if (GetKeyboardState(keystate) == 0) {return 0;}
				int ctrl_pressed = (keystate[VK_CONTROL] & 0x80);
				int shift_pressed = (keystate[VK_SHIFT] & 0x80);
				int alt_pressed = (keystate[VK_MENU] & 0x80);
				bool is_edit_session = (((1 << key_type) & MASK_NO_FN) == 0) && msg.wParam == (VK_F1 + key_type - F1) ? true
					: key_type == ALT && alt_pressed && msg.wParam == key_val ? true
					: key_type == CTRL && ctrl_pressed && !shift_pressed && msg.wParam == key_val ? true
					: key_type == CTRL_SHIFT && ctrl_pressed && shift_pressed && msg.wParam == key_val ? true
					: false;
				if (is_edit_session){
					HWND hwndSess = GetDlgItem(hwnd, IDCX_SESSIONTREEVIEW);
					TreeView_EditLabel(hwndSess, TreeView_GetSelection(hwndSess));
					continue;
				}
			}

			if (msg.wParam == VK_DOWN){
				SetFocus(GetDlgItem(hwnd,IDCX_SESSIONTREEVIEW));
			}
    	}
		if (showSessionTreeview && !isFreshingSessionTreeView && !dragging && hEdit == NULL)
		{
			char selected_sess[512] = { 0 };
			get_selected_session(GetDlgItem(hwnd, IDCX_SESSIONTREEVIEW), selected_sess, sizeof selected_sess);
			if (strcmp(selected_sess, ANDROID_DIR_FOLDER_NAME) == 0)
			{
				check_update_device();
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
	hConfigWnd = NULL;
	stop_adb_scan();
	SetActiveWindow(hwndparent);
    return ret;
}

/*
 * Null dialog procedure.
 */
static int CALLBACK NullDlgProc(HWND hwnd, UINT msg,
				WPARAM wParam, LPARAM lParam)
{
    return 0;
}

HTREEITEM session_treeview_insert(struct treeview_faff *faff,
				 int level, char *text, LPARAM flags, bool changed = false)
{
    TVINSERTSTRUCT ins;
    int i;
    HTREEITEM newitem;
    ins.hParent = (level > 0 ? faff->lastat[level - 1] : TVI_ROOT);
    ins.hInsertAfter = faff->lastat[level];
#if _WIN32_IE >= 0x0400 && defined NONAMELESSUNION
#define INSITEM DUMMYUNIONNAME.item
#else
#define INSITEM item
#endif
	ins.INSITEM.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIS_EXPANDED;
    ins.INSITEM.pszText = text;
    ins.INSITEM.cchTextMax = strlen(text)+1;
    ins.INSITEM.lParam = flags;
	ins.INSITEM.iImage = ((flags == SESSION_GROUP) ? 1 : 0) + (changed ? 3 : 0);
	ins.INSITEM.iSelectedImage = ((flags == SESSION_GROUP) ? 2 : 0) + (changed ? 3 : 0);
    newitem = TreeView_InsertItem(faff->treeview, &ins);
    
    faff->lastat[level] = newitem;
	for (i = level + 1; i < sizeof(faff->lastat) / sizeof(faff->lastat[0]); i++)
		faff->lastat[i] = NULL;
    return newitem;
}

static HTREEITEM treeview_insert(struct treeview_faff *faff,
				 int level, char *text, char *path)
{
    TVINSERTSTRUCT ins;
    int i;
    HTREEITEM newitem;
    ins.hParent = (level > 0 ? faff->lastat[level - 1] : TVI_ROOT);
    ins.hInsertAfter = faff->lastat[level];
#if _WIN32_IE >= 0x0400 && defined NONAMELESSUNION
#define INSITEM DUMMYUNIONNAME.item
#else
#define INSITEM item
#endif
    ins.INSITEM.mask = TVIF_TEXT | TVIF_PARAM;
    ins.INSITEM.pszText = text;
    ins.INSITEM.cchTextMax = strlen(text)+1;
    ins.INSITEM.lParam = (LPARAM)path;
    newitem = TreeView_InsertItem(faff->treeview, &ins);
    if (level > 0)
	TreeView_Expand(faff->treeview, faff->lastat[level - 1],
			/*(level > 1 ? TVE_COLLAPSE : TVE_EXPAND)*/TVE_EXPAND);
    faff->lastat[level] = newitem;
    for (i = level + 1; i < 4; i++)
	faff->lastat[i] = NULL;
    return newitem;
}

/*
 * Create the panelfuls of controls in the configuration box.
 */
static void create_controls(HWND hwnd, char * _path)
{
	char path[128] = { 0 };
    struct ctlpos cp;
    int index;
    int base_id;
    struct winctrls *wc;

	strncpy(path, _path, sizeof(path) - 1);
    if (!path[0]) {
	/*
	 * Here we must create the basic standard controls.
	 */
	ctlposinit(&cp, hwnd, 3, 3, TREEVIEW_HEIGHT + 16);
	wc = &ctrls_base;
	base_id = IDCX_STDBASE;
    } else {
	/*
	 * Otherwise, we're creating the controls for a particular
	 * panel.
	 */
	ctlposinit(&cp, hwnd, showSessionTreeview? (100 + SESSION_TREEVIEW_WIDTH) : 100, 3, 13);
	wc = &ctrls_panel;
	base_id = IDCX_PANELBASE;
    }

	std::vector<struct ctlpos> pos_stack;
    for (index=-1; (index = ctrl_find_path(ctrlbox, path, index)) >= 0 ;) {
		struct controlset *s = ctrlbox->ctrlsets[index];
		if (s->pop_pos && !pos_stack.empty()){ pos_stack.pop_back(); }
		if (s->push_pos){ pos_stack.push_back(cp); }
		if (s->use_pos && !pos_stack.empty()){ cp = pos_stack[pos_stack.size()-1]; }
		winctrl_layout(&dp, wc, &cp, s, &base_id);
    }
}
/*
 * extract item from session
 */
const char* extract_last_part(const char* session)
{
	const char* c = NULL;

	c = strrchr(session, '#');
	if (c){
		if (*(c + 1) == '\0')
		{
			for (int i = strlen(session) - 2; i >= 0; i--){
				if (session[i] == '#'){ return session + i + 1; }
			}
			return session;
		}
		else
		{
			return (c + 1);
		}
	}else{
		return session;
	}
}
/*
 * extract group from session
 */
void extract_group(const char* session, 
	char* group, const int glen)
{
	const char* c = NULL;

	c = strrchr(session, '#');
	if (c){
		memcpy(group, session, c - session + 1);
		group[c- session + 1] = '\0';
	}else{
		group[0] = '\0';
	}
	
}
/*
 * convert treeview to session
 */
LPARAM conv_tv_to_sess(
    HWND hwndSess, HTREEITEM hitem, 
    char* const sess_name, const int name_len)
{
    TVITEM item;
    HTREEITEM i = hitem; 
	char buffer[256];
	int item_len = 0;
	int left = name_len - 1;
	int sess_flags = SESSION_ITEM;
	
	memset(sess_name, '\0', name_len);
	do{
		item.hItem = i;
		item.pszText = buffer;
		item.cchTextMax = sizeof(buffer);
		item.mask = TVIF_PARAM | TVIF_TEXT;
		TreeView_GetItem(hwndSess, &item); 
		if (left == (name_len - 1) 
			&& item.lParam == SESSION_GROUP){
			left--;
			sess_name[left] = '#';
			sess_flags = SESSION_GROUP;
		}

		i = TreeView_GetParent(hwndSess, i);
		item_len = strlen(buffer);
		//no enough space, return empty session name
		if (item_len > left){
			return SESSION_NONE;
		}

		memcpy(sess_name + left - item_len, buffer, item_len);
		left = left - item_len - 1;
		sess_name[left] = '#';
	}while (i);
	strcpy(sess_name, sess_name + left + 1);
	return sess_flags;
}
/*
 * get selected session name configuration.
 */
LPARAM get_selected_session(HWND hwndSess, char* const sess_name, const int name_len)
{

	HTREEITEM hitem =
		TreeView_GetSelection(hwndSess);
    return conv_tv_to_sess(hwndSess, hitem,sess_name, name_len);
}

/*
 * handle edit message for session treeview.
 * return if the message belongs to the session treeview edit control
 */
static int edit_session_treeview(HWND hwndSess, int eflag)
{
	char buffer[256] = {0};
    
	char itemstr[64] = {0};
	char to_session[256] = {0};
	int i = 0;
	int pos = 0;
	char* c = NULL;
	TVITEM item;
	HTREEITEM hi;
	int sess_flags = SESSION_NONE;

    if (!hwndSess)
        return FALSE;

	switch(eflag){
	case EDIT_INIT:
		hEdit = NULL;
		return TRUE;
		break;
	case EDIT_BEGIN:
        /* get the pre_session */
		if (dragging){ drag_session_treeview(GetDlgItem(hwndSess, IDCX_SESSIONTREEVIEW), DRAG_CANCEL, 0, 0); }
		sess_flags = get_selected_session(hwndSess, pre_session, sizeof pre_session);
		if (is_pre_defined_session(pre_session) || sess_flags == SESSION_NONE){
			hEdit = NULL;
            TreeView_EndEditLabelNow(hwndSess, TRUE);
			return TRUE;
		}
		hEdit = TreeView_GetEditControl(hwndSess);
        return TRUE;
		break;

    case EDIT_CANCEL:
        if (hEdit == NULL || pre_session[0] == '\0')
			return FALSE;
        hEdit = NULL;
        TreeView_EndEditLabelNow(hwndSess, TRUE);
        return TRUE;
        break;
        
    case EDIT_OK:
        if (hEdit == NULL || pre_session[0] == '\0')
			return FALSE;
        TreeView_EndEditLabelNow(hwndSess, FALSE);
        return TRUE;
        break;
        
	case EDIT_END:
		if (hEdit == NULL || pre_session[0] == '\0')
			return FALSE;

		GetWindowText(hEdit, buffer, sizeof(buffer));

		/*validate the buffer*/ 
		if (buffer[0] == '\0')
			return TRUE;
		for (i = 0; i < strlen(buffer); i++){
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
        sess_flags = item.lParam;

		/* calc the to_session */
		strncpy(to_session, pre_session, sizeof to_session); 
		c = strrchr(to_session, '#');
		if (c)
			*(c + 1) = '\0';
		if (sess_flags == SESSION_GROUP) {
			*c = '\0';
			c = strrchr(to_session, '#');
			if (c)
				*(c + 1) = '\0';
		}
		pos = c ? (c - to_session + 1): 0;
		strncpy(to_session + pos, buffer, sizeof(to_session) - pos - 2);
		if (sess_flags == SESSION_GROUP) 
			strcat(to_session, "#");

		/* check if to session exists */
		struct sesslist sesslist;
		int first;
		int sessexist;
		get_sesslist(&sesslist, TRUE);
		first = lower_bound_in_sesslist(&sesslist, to_session);
		sessexist = first >= sesslist.nsessions ? FALSE 
            :(sess_flags == SESSION_ITEM)? (!strcmp(sesslist.sessions[first], to_session)) 
    		:(!strncmp(sesslist.sessions[first], to_session, strlen(to_session)));
		if (sessexist) {
			/* to_session exists */
			MessageBox(GetParent(hwndSess), "Destination session already exists.", "Error", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
			get_sesslist(&sesslist, FALSE);
			hEdit = NULL; 
			return TRUE;
		}

		/* now rename sessions */
		int cmplen = strlen(pre_session);
		if (sess_flags == SESSION_ITEM) {
			move_settings(pre_session, to_session);
		}else /* SESSION_GROUP */{
			first = lower_bound_in_sesslist(&sesslist, pre_session);
			char subsess[256];
			for (first = first; first < sesslist.nsessions; first++) {
				if (strncmp(sesslist.sessions[first], pre_session, cmplen)) 
					break;
				strncpy(subsess, to_session, sizeof subsess);	
				strncat(subsess + strlen(to_session), 
					sesslist.sessions[first] + cmplen, 
					sizeof(subsess) - strlen(to_session));	
				move_settings(sesslist.sessions[first], subsess);
			}
		}
		strncpy(pre_session, to_session, 256);
		conf_set_str(cfg, CONF_session_name, to_session);

		/* clean */
		get_sesslist(&sesslist, FALSE);
		hEdit = NULL;
				
		/* change the session treeview */
		//strncpy(itemstr, buffer, sizeof(itemstr));		
		//TreeView_SetItem(hwndSess, &item); 
		struct treeview_faff tvfaff;
		tvfaff.treeview = hwndSess;
		memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
		refresh_session_treeview(hwndSess, &tvfaff, to_session);

		break;
        return TRUE;
	}	
    return FALSE;
}

/*
 * delete session item/group
 */
static void del_session_treeview(HWND hwndSess, HTREEITEM selected_item, const char* session, int sess_flag)
{
    if (!strcmp(session, DEFAULT_SESSION_NAME)){
		gStorage->del_settings(session);
		strncpy(pre_session, DEFAULT_SESSION_NAME, sizeof(pre_session));
		load_settings(pre_session, cfg);
		dlg_refresh(NULL, &dp);
        return;
    }

    if (sess_flag == SESSION_ITEM) {
		gStorage->del_settings(session);
	}else if (sess_flag == SESSION_GROUP){
    	struct sesslist sesslist;
		int first;
        int cmplen = strlen(session);

		get_sesslist(&sesslist, TRUE);
		first = lower_bound_in_sesslist(&sesslist, session);
		for (first = first; first < sesslist.nsessions; first++) {
			if (strncmp(sesslist.sessions[first], session, cmplen)) 
				break;
			gStorage->del_settings(sesslist.sessions[first]);
		}
        get_sesslist(&sesslist, FALSE);
	}
    /* clear pre_session, don't change it before if sess_flag == SESSION_ITEM*/
    pre_session[0] = '\0';
    
    /* delete in the tree view, the next item will be selected automatically*/
    TreeView_DeleteItem(hwndSess, selected_item);
    return;
}

/*
 * backup session item/group
 */
static void backup_session_treeview(HWND hwndSess, HTREEITEM selected_item, const char* session, int sess_flag)
{
	BROWSEINFO bi;
    ZeroMemory(&bi,sizeof(BROWSEINFO));
	bi.lpszTitle = TEXT("Select a folder. A new folder 'putty_sessions' will be created under it.");
	//bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    TCHAR path[MAX_PATH];
    if(pidl == NULL){
    	return ;  
    }
	SHGetPathFromIDList(pidl,path);
	lstrcat(path, TEXT("\\putty_sessions"));
	
    if (sess_flag == SESSION_ITEM) {
		backup_settings(session, path);
	}else if (sess_flag == SESSION_GROUP){
    	struct sesslist sesslist;
		int first;
        int cmplen = strlen(session);

		get_sesslist(&sesslist, TRUE);
		first = lower_bound_in_sesslist(&sesslist, session);
		for (first = first; first < sesslist.nsessions; first++) {
			if (strncmp(sesslist.sessions[first], session, cmplen)) 
				break;
			backup_settings(sesslist.sessions[first], path);
		}
        get_sesslist(&sesslist, FALSE);
	}else{
		struct sesslist sesslist;
		get_sesslist(&sesslist, TRUE);
		for (int i = 0; i < sesslist.nsessions; i++) {
			backup_settings(sesslist.sessions[i], path);
		}
        get_sesslist(&sesslist, FALSE);
	}
    return;
}

/*
 * backup session item/group
 */
static void restore_session_treeview(HWND hwndSess, HTREEITEM selected_item, const char* session, int sess_flag)
{
	OPENFILENAME ofn;
	TCHAR szOpenFileNames[800*MAX_PATH];
	TCHAR szPath[MAX_PATH];
	TCHAR* session_name;
	int nLen = 0;
	ZeroMemory( &ofn, sizeof(ofn) );
	ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szOpenFileNames;
	ofn.nMaxFile = sizeof(szOpenFileNames);
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = TEXT("The Session Files(*.*)\0*.*\0");
	if( !GetOpenFileName( &ofn ) )
	{  
		return;
	}

	lstrcpyn(szPath, szOpenFileNames, ofn.nFileOffset );
	szPath[ ofn.nFileOffset ] = '\0';
	nLen = lstrlen(szPath);
  
	if( szPath[nLen-1] != '\\' )
	{
		lstrcat(szPath, TEXT("\\"));
	}
	FileStore fileStore(szPath);
  
	session_name = szOpenFileNames + ofn.nFileOffset;
	while( *session_name )
	{   
		Conf* tmpCfg = conf_new();
		char* unmunged = fileStore.unmungestr(session_name);
		load_settings(unmunged, tmpCfg, &fileStore);
		save_settings(unmunged, tmpCfg);
		if (session != NULL && 0 == strcmp(unmunged, session))
		{
			//reset current session's config 
			//in case it is reset in change_selected_session
			conf_copy_into(cfg, tmpCfg);

		}
		conf_free(tmpCfg);
		sfree(unmunged);
		session_name += lstrlen(session_name) +1; 
	}

	char selected_session[256] = { 0 };
	if (session != NULL)
	{
		strncpy(selected_session, session, sizeof(selected_session));
	}
	struct treeview_faff tvfaff;
	tvfaff.treeview = hwndSess;
	memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
	refresh_session_treeview(hwndSess, &tvfaff, selected_session);
}

static void refresh_tree_view(const char* old_session, const char* new_session, HWND treeview = NULL)
{
	HTREEITEM hfirst = NULL;
	int i;
	char *path = NULL;

	if (strcmp(old_session, ANDROID_DIR_FOLDER_NAME) && strcmp(new_session, ANDROID_DIR_FOLDER_NAME)
		&& strcmp(old_session, GLOBAL_SESSION_NAME) && strcmp(new_session, GLOBAL_SESSION_NAME))
		return;

	isFreshingTreeView = true;
	char *filter_str = strcmp(new_session, ANDROID_DIR_FOLDER_NAME) == 0 ? ANDROID_SETTING_NAME 
		: strcmp(new_session, GLOBAL_SESSION_NAME) == 0 ? SHORTCUT_SETTING_NAME
		: NULL;

	struct treeview_faff tvfaff;
	if (treeview == NULL) {treeview = GetDlgItem(hConfigWnd, IDCX_TREEVIEW); }
	tvfaff.treeview = treeview;
	memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
	TreeView_DeleteAllItems(treeview);

	for (i = 0; i < ctrlbox->nctrlsets; i++) {
		struct controlset *s = ctrlbox->ctrlsets[i];
		HTREEITEM item;
		int j;
		char *c;

		if (!s->pathname[0])
			continue;
		j = path ? ctrl_path_compare(s->pathname, path) : 0;
		if (j == INT_MAX)
			continue;	       /* same path, nothing to add to tree */

		if (s->pathname[0] == ' ' && filter_str == NULL)
			continue;
		if (filter_str != NULL && memcmp(s->pathname, filter_str, strlen(filter_str)))
			continue;
		/*
		* We expect never to find an implicit path
		* component. For example, we expect never to see
		* A/B/C followed by A/D/E, because that would
		* _implicitly_ create A/D. All our path prefixes
		* are expected to contain actual controls and be
		* selectable in the treeview; so we would expect
		* to see A/D _explicitly_ before encountering
		* A/D/E.
		*/
		assert(j == ctrl_path_elements(s->pathname) - 1);

		c = strrchr(s->pathname, '/');
		if (!c)
			c = s->pathname;
		else
			c++;

		item = treeview_insert(&tvfaff, j, c, s->pathname);
		if (!hfirst)
			hfirst = item;

		path = s->pathname;
	}
	isFreshingTreeView = false;

	/*
	* Put the treeview selection on to the Session panel.
	* This should also cause creation of the relevant
	* controls.
	*/
	TreeView_SelectItem(tvfaff.treeview, hfirst);
}

/*
 * Save previous session's configuration and load the current's configuration.
 */
static LPARAM change_selected_session(HWND hwndSess)
{
    char sess_name[256];
	int isdef;
	LPARAM selected_flags;
	
	selected_flags = get_selected_session(hwndSess, sess_name, 256);
    if (sess_name[0] == '\0') {
		strcpy(sess_name,DEFAULT_SESSION_NAME); 
	}
	if (!strcmp(sess_name, pre_session)) {
		return selected_flags;
	}

    isdef = 0 /*!strcmp(pre_session, DEFAULT_SESSION_NAME)*/;
	if (pre_session[0] != 0 && !isdef)
	{
        char *errmsg = save_settings(pre_session, (Conf*)dp.data);
        if (errmsg) {
            dlg_error_msg(&dp, errmsg);
            sfree(errmsg);
        }
	}
	refresh_tree_view(pre_session, sess_name);
	strncpy(pre_session, sess_name, 256);
    load_settings(sess_name, cfg);
	struct winctrl *wc = dlg_findbyctrl(&dp, ctrlbox->okbutton);
	if (wc){
		HWND h = GetDlgItem(dp.hwnd, wc->base_id);
		SetWindowText(h, selected_flags == SESSION_GROUP ? ("Open Sub-...") : ("Open"));
	}
	dlg_refresh(NULL, &dp);
    SetFocus(hwndSess);

	int is_adb_folder = !strcmp(pre_session, ANDROID_DIR_FOLDER_NAME);
	if (is_adb_folder){ start_adb_scan(); }
	else{ stop_adb_scan(); }

	return selected_flags;
}

/*
 * Create the session tree view.
 */
static HWND create_session_treeview(HWND hwnd, struct treeview_faff* tvfaff)
{
    RECT r;
    WPARAM font;
    HWND tvstatic;
	HWND searchbar;
	HWND sessionview;
	HIMAGELIST hImageList;
	HBITMAP hBitMap;
	int i;

	for (i = 0; i < sizeof(st_popup_menus)/sizeof(HMENU); i++)
		st_popup_menus[i] = CreatePopupMenu();
	AppendMenu(st_popup_menus[SESSION_NONE], MF_ENABLED, IDM_ST_NEWSESS, "New Session");
	AppendMenu(st_popup_menus[SESSION_NONE], MF_ENABLED, IDM_ST_NEWGRP, "New Group");
	AppendMenu(st_popup_menus[SESSION_NONE], MF_SEPARATOR, 0, 0);
	AppendMenu(st_popup_menus[SESSION_NONE], MF_ENABLED, IDM_ST_BACKUP_ALL, "Export All...");
	AppendMenu(st_popup_menus[SESSION_NONE], MF_ENABLED, IDM_ST_RESTORE, "Import...");

	AppendMenu(st_popup_menus[SESSION_DEFAULT], MF_ENABLED, IDM_ST_DEL, "Reset Default");
	AppendMenu(st_popup_menus[SESSION_DEFAULT], MF_SEPARATOR, 0, 0);
	AppendMenu(st_popup_menus[SESSION_DEFAULT], MF_ENABLED, IDM_ST_NEWSESS, "New Session");
	AppendMenu(st_popup_menus[SESSION_DEFAULT], MF_ENABLED, IDM_ST_NEWGRP, "New Group");
	AppendMenu(st_popup_menus[SESSION_DEFAULT], MF_SEPARATOR, 0, 0);
	AppendMenu(st_popup_menus[SESSION_DEFAULT], MF_ENABLED, IDM_ST_BACKUP_ALL, "Export All...");
	AppendMenu(st_popup_menus[SESSION_DEFAULT], MF_ENABLED, IDM_ST_RESTORE, "Import...");
	
	//AppendMenu(st_popup_menus[SESSION_GROUP], MF_ENABLED, IDM_ST_NEWSESSONGRP, "New Session Base On");
	AppendMenu(st_popup_menus[SESSION_GROUP], MF_ENABLED, IDM_ST_DUPGRP, "Duplicate Group");
	AppendMenu(st_popup_menus[SESSION_GROUP], MF_ENABLED, IDM_ST_DEL, "Delete");
	AppendMenu(st_popup_menus[SESSION_GROUP], MF_SEPARATOR, 0, 0);
	AppendMenu(st_popup_menus[SESSION_GROUP], MF_ENABLED, IDM_ST_BACKUP, "Export Group...");
	AppendMenu(st_popup_menus[SESSION_GROUP], MF_SEPARATOR, 0, 0);
	AppendMenu(st_popup_menus[SESSION_GROUP], MF_ENABLED, IDM_ST_NEWSESS, "New Session");
	AppendMenu(st_popup_menus[SESSION_GROUP], MF_ENABLED, IDM_ST_NEWGRP, "New Group");
	AppendMenu(st_popup_menus[SESSION_GROUP], MF_SEPARATOR, 0, 0);
	AppendMenu(st_popup_menus[SESSION_GROUP], MF_ENABLED, IDM_ST_BACKUP_ALL, "Export All...");
	AppendMenu(st_popup_menus[SESSION_GROUP], MF_ENABLED, IDM_ST_RESTORE, "Import...");
	
	AppendMenu(st_popup_menus[SESSION_ITEM], MF_ENABLED, IDM_ST_DUPSESS, "Duplicate Session");
	AppendMenu(st_popup_menus[SESSION_ITEM], MF_ENABLED, IDM_ST_DEL, "Delete");
	AppendMenu(st_popup_menus[SESSION_ITEM], MF_SEPARATOR, 0, 0);
	AppendMenu(st_popup_menus[SESSION_ITEM], MF_ENABLED, IDM_ST_BACKUP, "Export Session...");
	AppendMenu(st_popup_menus[SESSION_ITEM], MF_SEPARATOR, 0, 0);
	AppendMenu(st_popup_menus[SESSION_ITEM], MF_ENABLED, IDM_ST_NEWSESS, "New Session");
	AppendMenu(st_popup_menus[SESSION_ITEM], MF_ENABLED, IDM_ST_NEWGRP, "New Group");
	AppendMenu(st_popup_menus[SESSION_ITEM], MF_SEPARATOR, 0, 0);
	AppendMenu(st_popup_menus[SESSION_ITEM], MF_ENABLED, IDM_ST_BACKUP_ALL, "Export All...");
	AppendMenu(st_popup_menus[SESSION_ITEM], MF_ENABLED, IDM_ST_RESTORE, "Import...");

    r.left = 3;
    r.right = r.left + SESSION_TREEVIEW_WIDTH - 6;
    r.top = 3;
    r.bottom = r.top + 10;
    MapDialogRect(hwnd, &r);
	const int SEARCH_TEXT_LEN = 50;
    tvstatic = CreateWindowEx(0, "STATIC", "&Search:",
			      WS_CHILD | WS_VISIBLE,
			      r.left, r.top,
			      SEARCH_TEXT_LEN, r.bottom - r.top,
			      hwnd, (HMENU) IDCX_TVSTATIC, hinst,
			      NULL);
	searchbar = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
			      WS_CHILD | WS_VISIBLE,
			      r.left + SEARCH_TEXT_LEN, 1,
			      r.right - r.left - SEARCH_TEXT_LEN, r.bottom - 1,
			      hwnd, (HMENU) IDCX_SEARCHBAR, hinst,
			      NULL);
    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
    SendMessage(tvstatic, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

    r.left = 3;
    r.right = r.left + SESSION_TREEVIEW_WIDTH - 6;
    r.top = 13;
    r.bottom = r.top + TREEVIEW_HEIGHT;
    MapDialogRect(hwnd, &r);
    sessionview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
			      WS_CHILD | WS_VISIBLE |
			      WS_TABSTOP | TVS_HASLINES |
			      TVS_HASBUTTONS | TVS_LINESATROOT | TVS_EDITLABELS |
			      TVS_SHOWSELALWAYS, r.left, r.top,
			      r.right - r.left, r.bottom - r.top,
			      hwnd, (HMENU) IDCX_SESSIONTREEVIEW, hinst,
			      NULL);
    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
    SendMessage(sessionview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));
    tvfaff->treeview = sessionview;
    memset(tvfaff->lastat, 0, sizeof(tvfaff->lastat));

	hImageList = ImageList_Create(16,16,ILC_COLOR16,6,10);
	hBitMap = LoadBitmap(hinst,MAKEINTRESOURCE(IDB_TREE));
	ImageList_Add(hImageList,hBitMap,NULL);
	DeleteObject(hBitMap);
	SendDlgItemMessage(hwnd, IDCX_SESSIONTREEVIEW, TVM_SETIMAGELIST,0,(LPARAM)hImageList); 

	return sessionview;
}

/**
 * return if need to reload session
 */
int load_sessions_from_others(struct sesslist* sesslist);


/*
 * Set up the session view contents.
 */

static void refresh_session_treeview(
    HWND sessionview, 
    struct treeview_faff* tvfaff, 
    const char* select_session)
{
	HTREEITEM hfirst = NULL;
	HTREEITEM item = NULL;
    int i, j, k;               //index to iterator all the characters of the sessions
	int level;              //tree item's level
	int b;                  //index of the tree item's first character
	char itemstr[64];
	char selected_session_name[256] = {0};
	char lower_session_name[256] = { 0 }; 
	char pre_show_session_name[256] = { 0 };
    struct sesslist sesslist;
    int is_select;
    char session[256] = {0};
    HTREEITEM pre_grp_item = NULL;
    int pre_grp_collapse = 0;

	
	char filter[64] = {0};
	HWND hwndSearchBar = GetDlgItem(dp.hwnd,IDCX_SEARCHBAR);
	GetWindowText(hwndSearchBar, filter, sizeof(filter));
	for (int m = 0; m < sizeof(filter) && m < strlen(filter); m++)
		filter[m] = tolower(filter[m]);

	save_settings(pre_session, cfg);

	isFreshingSessionTreeView = true;
    memset(tvfaff->lastat, 0, sizeof(tvfaff->lastat));
	TreeView_DeleteAllItems(tvfaff->treeview);

    get_sesslist(&sesslist, TRUE);
	if (load_sessions_from_others(&sesslist)){
		get_sesslist(&sesslist, FALSE);
		get_sesslist(&sesslist, TRUE);
	}
	
    for (i = 0; i < sesslist.nsessions; i++){
		if (!sesslist.sessions[i][0])
			continue;
	
		if (strcmp(sesslist.sessions[i], OTHER_SESSION_NAME) == 0){
			continue;
		}

        strncpy(lower_session_name, sesslist.sessions[i], sizeof(lower_session_name));
		for (int m = 0; m < sizeof(lower_session_name) && m < strlen(lower_session_name); m++){
			lower_session_name[m] = tolower(lower_session_name[m]);
		}

		if (filter[0] && !strstr(lower_session_name, filter)){
			continue;
		}
        is_select = !strcmp(sesslist.sessions[i], select_session);
        strncpy(session, sesslist.sessions[i], sizeof(session));
        
		level = 0;
		b = 0;
		for (j = 0, k = 0; sesslist.sessions[i][j] && level < 10; j++, k++){
			if (sesslist.sessions[i][j] == '#'){
				if (b == j){
					b = j + 1;
					continue;
				}

				level++;
				if (i == 0 || strncmp(pre_show_session_name, sesslist.sessions[i], j + 1)){
					int len = (j - b) > 63 ? 63 : (j-b);
					strncpy(itemstr, sesslist.sessions[i]+b, len);
					itemstr[len] = '\0';
					item = session_treeview_insert(tvfaff, level-1, itemstr, SESSION_GROUP);

                    //we can only expand a group with a child
                    //so we expand the previous group
                    //leave the group in tail alone.
                    if (pre_grp_item){
                        TreeView_Expand(tvfaff->treeview, pre_grp_item,
                    			((pre_grp_collapse&& filter[0] == 0) ? TVE_COLLAPSE : TVE_EXPAND));
                    }
                    pre_grp_item = item;
                    char grp_session[256] = {0};
                    strncpy(grp_session, session, j + 1);
                    pre_grp_collapse = load_isetting(grp_session, GRP_COLLAPSE_SETTING, 1);
                	
				}
				b = j + 1;
			}
		}
		strncpy(pre_show_session_name, sesslist.sessions[i], sizeof(pre_show_session_name));
		
		if (b == j){
            if (is_select) {
				hfirst = item;
				strncpy(selected_session_name, sesslist.sessions[i], sizeof(selected_session_name));
            }
			continue;
		}
        item = session_treeview_insert(tvfaff, level, sesslist.sessions[i]+b, SESSION_ITEM);
        if (pre_grp_item){
            TreeView_Expand(tvfaff->treeview, pre_grp_item,
        			((pre_grp_collapse&& filter[0] == 0)? TVE_COLLAPSE : TVE_EXPAND));
            pre_grp_item = NULL;
        }
        
        if (is_select) {
			hfirst = item;
			strncpy(selected_session_name, sesslist.sessions[i], sizeof(selected_session_name));
        }
        
	    if (!hfirst){
	        hfirst = item;
			strncpy(selected_session_name, sesslist.sessions[i], sizeof(selected_session_name));
	    }
	}
	isFreshingSessionTreeView = false;
	InvalidateRect(sessionview, NULL, TRUE);
	if (hfirst){
	    TreeView_SelectItem(sessionview, hfirst);
	    change_selected_session(sessionview);
	}else{
		strncpy(pre_session, DEFAULT_SESSION_NAME, sizeof(pre_session));
		load_settings(pre_session, cfg);
		dlg_refresh(NULL, &dp);
	}
	get_sesslist(&sesslist, FALSE);
}

void on_sessions_changed()
{
	if (hConfigWnd == NULL){ return; }
	struct treeview_faff tvfaff;
	tvfaff.treeview = GetDlgItem(hConfigWnd, IDCX_SESSIONTREEVIEW);
	memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
	refresh_session_treeview(tvfaff.treeview, &tvfaff, "");
}

/*
 * copy session, return FALSE if to_session exist
 */
int copy_session(
    struct sesslist* sesslist, 
    const char* from_session, 
    const char* to_session, 
    int to_sess_flag)
{
    int pos, sessexist;
    
    pos = lower_bound_in_sesslist(sesslist, to_session);
	sessexist = pos >= sesslist->nsessions ? FALSE 
        :(to_sess_flag == SESSION_ITEM)? (!strcmp(sesslist->sessions[pos], to_session)) 
		: (!strncmp(sesslist->sessions[pos], to_session, strlen(to_session)));
    if (sessexist) return FALSE;

    copy_settings(from_session, to_session);
    return TRUE;
}

int copy_item_under_group(const char* session_name, const char* origin_group, const char* dest_group)
{
	char to_session[256] = { 0 };
	snprintf(to_session, sizeof(to_session)-1, "%s%s", dest_group, session_name + strlen(origin_group));
	copy_settings(session_name, to_session);
	return 1;
}
/*
 * duplicate session item with index
 */
void dup_session_treeview(
    HWND hwndSess,  
    HTREEITEM selected_item,
    const char* from_session, 
    const char* to_session_pre, 
	int from_sess_flag,
    int to_sess_flag)
{
    struct sesslist sesslist;
    char to_session[256] = {0};
    int sess_len, append_index;
    int success;
    
    get_sesslist(&sesslist, TRUE);
    
    append_index = sesslist.nsessions; 
    snprintf(to_session, sizeof(to_session) - 2,
             "%s %d", to_session_pre, append_index);
    if (to_sess_flag == SESSION_GROUP){
        strcat(to_session, "#");
    }

	success = copy_session(&sesslist, from_session, to_session, to_sess_flag);
    while(!success) {
        sess_len = strlen(to_session);
        if (sess_len > (sizeof(to_session) - 10)){
			MessageBox(hwndSess, "Session name is too long! \nAre you fucking me?", "Error", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
            get_sesslist(&sesslist, FALSE);
            return;
        }
        if (to_sess_flag == SESSION_GROUP)
            to_session[sess_len - 1] = '\0';
        strcat(to_session, " ");
        if (to_sess_flag == SESSION_GROUP)
            strcat(to_session, "#");
	    success = copy_session(&sesslist, from_session, to_session, to_sess_flag);
    }
    get_sesslist(&sesslist, FALSE);
	if (to_sess_flag == SESSION_GROUP && from_sess_flag == SESSION_GROUP){
		for_grouped_session_do(from_session, boost::bind(copy_item_under_group, _1, from_session, to_session), 100);
	}

    struct treeview_faff tvfaff;
    tvfaff.treeview = hwndSess;
    memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
    refresh_session_treeview(hwndSess, &tvfaff, to_session);
    TreeView_EditLabel(hwndSess, TreeView_GetSelection(hwndSess));
}

/*
 * show popup memu.
 */
static void show_st_popup_menu(HWND  hwndSess)
{
	POINT screen_pos;
	POINT client_pos;
	TVHITTESTINFO tvht;
	HTREEITEM hit_item;
	int sess_flags;

	GetCursorPos(&screen_pos);
	client_pos = screen_pos;
	ScreenToClient(hwndSess, &client_pos);
	tvht.pt.x = client_pos.x;
	tvht.pt.y = client_pos.y;
	hit_item = TreeView_HitTest(hwndSess, (LPARAM)&tvht);
	if (hit_item){
		TreeView_Select(hwndSess, hit_item, TVGN_CARET);
		sess_flags = get_selected_session(hwndSess, pre_session, sizeof pre_session);
		if (!strcmp(pre_session, DEFAULT_SESSION_NAME)){
			sess_flags = SESSION_DEFAULT;
		}
	}
	else{
		sess_flags = SESSION_NONE;
	}


	int menuid = TrackPopupMenu(st_popup_menus[sess_flags],
		TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		screen_pos.x, screen_pos.y,
		0, hwndSess, NULL);
	if (menuid <= IDM_ST_NONE || menuid >= IDM_ST_ROOF)
		return;

	extern void handle_popup_menu(int menuid, HWND  hwndSess, HTREEITEM hit_item, int sess_flags);
	handle_popup_menu(menuid, hwndSess, hit_item, sess_flags);
}

void handle_popup_menu(int menuid, HWND  hwndSess, HTREEITEM hit_item, int sess_flags)
{
    /* now we handle the message */
    if (menuid == IDM_ST_DEL){
        del_session_treeview(hwndSess, hit_item, pre_session, sess_flags);
        return;
    }
	if (menuid == IDM_ST_BACKUP){
		backup_session_treeview(hwndSess, hit_item, pre_session, sess_flags);
        return;
	}
	if (menuid == IDM_ST_BACKUP_ALL){
		backup_session_treeview(hwndSess, hit_item, pre_session, SESSION_NONE);
        return;
	}
	if (menuid == IDM_ST_RESTORE){
		restore_session_treeview(hwndSess, hit_item, pre_session, sess_flags);
        return;
	}

    /* we need to new a session */
    char base_session[256] = {0};
    char to_session[256] = {0};
	char hit_group[256] = { 0 };
	strncpy(hit_group, pre_session, sizeof(hit_group)-2);
	char* ch = NULL;
	if ((ch = strrchr(hit_group, '#')) == NULL){ hit_group[0] = '\0'; }
	else{ *(ch + 1) = '\0'; }
    int to_sess_flag = SESSION_NONE;
	int from_sess_flag = SESSION_ITEM;
    switch (menuid){
        case IDM_ST_NEWSESS:
			strncpy(base_session, hit_group[0] ? hit_group : DEFAULT_SESSION_NAME, sizeof base_session);
			if (hit_group[0])
			{
				snprintf(to_session, sizeof(to_session)-1, "%s%s", hit_group, extract_last_part(hit_group));
				to_session[strlen(to_session) - 1] = '\0';
			}
			else
			{
				strncpy(to_session, "Session", sizeof(to_session)-1);
			}
            to_sess_flag = SESSION_ITEM;
            break;
        case IDM_ST_NEWGRP:
			strncpy(base_session, hit_group[0] ? hit_group : DEFAULT_SESSION_NAME, sizeof base_session);
			snprintf(to_session, sizeof(to_session)-1, "%sGroup", hit_group);
            to_sess_flag = SESSION_GROUP;
            break;
        case IDM_ST_DUPSESS:
            save_settings(pre_session, (Conf*)dp.data);
            strncpy(base_session, pre_session, sizeof base_session);
            strncpy(to_session, pre_session, sizeof to_session);
            strncat(to_session, " Session", sizeof(to_session) - strlen(pre_session));
            to_sess_flag = SESSION_ITEM;
            break;
        case IDM_ST_DUPGRP:
            save_settings(pre_session, (Conf*)dp.data);
            strncpy(base_session, pre_session, sizeof base_session);
            strncpy(to_session, pre_session, sizeof to_session);
            to_session[strlen(to_session)-1] = '\0';
            strncat(to_session, " Group", sizeof(to_session) - strlen(pre_session));
            to_sess_flag = SESSION_GROUP;
			from_sess_flag = SESSION_GROUP;
            break;
        case IDM_ST_NEWSESSONGRP:
            save_settings(pre_session, (Conf*)dp.data);
            strncpy(base_session, pre_session, sizeof base_session);
            strncpy(to_session, pre_session, sizeof to_session);
            strncat(to_session, "Session", sizeof(to_session)-strlen(pre_session));
            to_sess_flag = SESSION_ITEM;
            break;
    };
    dup_session_treeview(hwndSess, hit_item, base_session, to_session, from_sess_flag,to_sess_flag);
    
/*    
	char buf[10];
	sprintf(buf, "%d", menuid);
	MessageBox(hwnd, buf, "Error",MB_OK|MB_ICONINFORMATION);
	
	*/
}
/*
 * drag session treeview.
 * return if the msg is handled
 */
int drag_session_treeview(HWND hwndSess, int flags, WPARAM wParam, LPARAM lParam)
{
	int curnum = 0;
	HTREEITEM htiTarget;
	if (hCopyCurs == NULL)
		hCopyCurs =  CreateCursor( NULL,   // app. instance 
             8,                // horizontal position of hot spot 
             8,                 // vertical position of hot spot 
             16,                // cursor width 
             16,                // cursor height 
             ANDmaskCursor,     // AND mask 
             XORmaskCursor );   // XOR mask 
	
	if (flags == DRAG_CTRL_DOWN 
		&& ((lParam & (1<<29)) == 0)){
		if (!dragging) return FALSE;
		SetCursor(hCopyCurs);
		do{ curnum = ShowCursor(TRUE); } while (curnum < 0); 
		while(curnum > 0) curnum = ShowCursor(FALSE); 
	}else if (flags == DRAG_CTRL_UP){
		if (!dragging) return FALSE;
        do{ curnum = ShowCursor(FALSE); } while (curnum >= 0); 
		while(curnum < -1) curnum = ShowCursor(TRUE); 
	}else if (flags == DRAG_CANCEL){
		if (!dragging) return FALSE;
        ImageList_DragLeave(hwndSess);
		ImageList_EndDrag(); 
        TreeView_SelectDropTarget(hwndSess, NULL);
		ReleaseCapture(); ShowCursor(TRUE); dragging = FALSE;
        return TRUE;
	}else if (flags == DRAG_BEGIN){
		HIMAGELIST himl;
		RECT rcItem;
		LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW) lParam;

        save_settings(pre_session, (Conf*)dp.data);
		if (is_pre_defined_session(pre_session)){
			dragging = FALSE;
			return FALSE;
		}
	    /* select the session. End dragging if it is not item.*/
        TreeView_SelectItem(hwndSess, lpnmtv->itemNew.hItem);
        //if (lpnmtv->itemNew.lParam != SESSION_ITEM) 
        //    return TRUE;
        
        SetFocus(hwndSess);
		/*
		 * Tell the tree-view control to create an image to use 
    	 * for dragging. 
		 */
		himl = TreeView_CreateDragImage(hwndSess, lpnmtv->itemNew.hItem); 

		/* Get the bounding rectangle of the item being dragged. */
		TreeView_GetItemRect(hwndSess, lpnmtv->itemNew.hItem, &rcItem, TRUE); 

		/* Start the drag operation.*/ 
		ImageList_BeginDrag(himl, 0, 0, 0);
		ImageList_DragEnter(hwndSess, lpnmtv->ptDrag.x, lpnmtv->ptDrag.x); 

		while(ShowCursor(FALSE)>=0); 
		SetCapture(GetParent(hwndSess)); 
		dragging = TRUE;
		return TRUE;

	} else if (flags == DRAG_MOVE){
		if (!dragging) return FALSE;

		if(GetKeyState(MK_LBUTTON) >= 0){
			drag_session_treeview(hwndSess, DRAG_CANCEL, 0, 0);
			return FALSE;
		}

		TVHITTESTINFO tvht;
        POINTS pos;
		/* 
		 * Drag the item to the current position of the mouse pointer. 
         * First convert the dialog coordinates to control coordinates. 
		 */
		pos = MAKEPOINTS(lParam);
		POINT point;
		point.x = pos.x;
		point.y = pos.y;
		ClientToScreen(GetParent(hwndSess), &point);
		ScreenToClient(hwndSess, &point);
		ImageList_DragMove(point.x + 2, point.y + 2);
		/* Turn off the dragged image so the background can be refreshed. */
		ImageList_DragShowNolock(FALSE); 
			
		// Find out if the pointer is on the item. If it is, 
		// highlight the item as a drop target. 
		tvht.pt.x = point.x; 
		tvht.pt.y = point.y; 
		if ((htiTarget = TreeView_HitTest(hwndSess, &tvht)) != NULL) { 
			TreeView_SelectDropTarget(hwndSess, htiTarget); 
		} 
		ImageList_DragShowNolock(TRUE);

		return TRUE;
	} else if (flags == DRAG_END){
		if (!dragging) return FALSE;

        ImageList_DragLeave(hwndSess);
		ImageList_EndDrag(); 

        if ((htiTarget = TreeView_GetDropHilight(hwndSess))== NULL){
			TreeView_SelectDropTarget(hwndSess, NULL);
			ReleaseCapture(); ShowCursor(TRUE); dragging = FALSE;
            return TRUE;
        }

		/* calc to_session */
        char to_session[256] = {0};
        int sess_flag;
		struct sesslist sesslist;

        sess_flag = conv_tv_to_sess(hwndSess, htiTarget, 
			to_session, sizeof to_session);
        if (sess_flag == SESSION_NONE) {
			TreeView_SelectDropTarget(hwndSess, NULL);
			ReleaseCapture(); ShowCursor(TRUE); dragging = FALSE;
			return TRUE;
		}
		extract_group(to_session, to_session, sizeof to_session);
        strncat(to_session, extract_last_part(pre_session), sizeof(to_session));

		if (!strcmp(pre_session, to_session) || !strncmp(pre_session, to_session, strlen(pre_session))){
			TreeView_SelectDropTarget(hwndSess, NULL);
			ReleaseCapture(); ShowCursor(TRUE); dragging = FALSE;
			return TRUE;
		}

		get_sesslist(&sesslist, TRUE);
		if (!copy_session(&sesslist, pre_session, to_session, SESSION_ITEM)){
			get_sesslist(&sesslist, FALSE);
			TreeView_SelectDropTarget(hwndSess, NULL);
			ReleaseCapture(); ShowCursor(TRUE); dragging = FALSE;
			MessageBox(GetParent(hwndSess), "Destination session already exists."
				, "Error", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
			return TRUE;
		} 
		get_sesslist(&sesslist, FALSE);
		if (pre_session[strlen(pre_session) - 1] == '#'){
			for_grouped_session_do(pre_session, boost::bind(copy_item_under_group, _1, pre_session, to_session), 100);
		}
		
		if ((wParam & MK_CONTROL) == 0){
			if (pre_session[strlen(pre_session) - 1] == '#'){
				extern int del_settings(const char *sessionname);
				for_grouped_session_do(pre_session, del_settings, 100);
			}
			gStorage->del_settings(pre_session);
		}
		strncpy(pre_session, to_session, sizeof pre_session);

        TreeView_SelectDropTarget(hwndSess, NULL);
		ReleaseCapture(); ShowCursor(TRUE); dragging = FALSE;

		struct treeview_faff tvfaff;
		tvfaff.treeview = hwndSess;
		memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
		refresh_session_treeview(hwndSess, &tvfaff, to_session);

		return TRUE;
	}
	return FALSE;
}

void create_session(union control *ctrl, void *dlg,
			  void *data, int event)
{
	if (event == EVENT_ACTION) {
		HWND hwndSess = GetDlgItem(hConfigWnd, IDCX_SESSIONTREEVIEW);
		save_settings(pre_session, (Conf*)dp.data);
		handle_popup_menu(IDM_ST_NEWSESS, hwndSess, NULL, SESSION_ITEM);
	}
}

void fork_session(union control *ctrl, void *dlg,
			  void *data, int event)
{
	if (event == EVENT_ACTION) {
		HWND hwndSess = GetDlgItem(hConfigWnd, IDCX_SESSIONTREEVIEW);
		save_settings(pre_session, (Conf*)dp.data);
		handle_popup_menu(IDM_ST_DUPSESS, hwndSess, NULL, SESSION_ITEM);
	}
}
void delete_item(union control *ctrl, void *dlg,
			  void *data, int event)
{
	if (event == EVENT_ACTION) {
		extern HWND hConfigWnd;
		HWND hwndSess = GetDlgItem(hConfigWnd, IDCX_SESSIONTREEVIEW);
 		HTREEITEM hitem = TreeView_GetSelection(hwndSess);
    	int sess_flags = conv_tv_to_sess(hwndSess, hitem, pre_session, sizeof pre_session);
		del_session_treeview(hwndSess, hitem, pre_session, sess_flags);
	}
}
void export_all(union control *ctrl, void *dlg,
			  void *data, int event)
{
	if (event == EVENT_ACTION) {
		//extern HWND hConfigWnd;
		//HWND hwndSess = GetDlgItem(hConfigWnd, IDCX_SESSIONTREEVIEW);
    	//backup_session_treeview(hwndSess, NULL, NULL, SESSION_NONE);
		
		//extern void upload_sessions();
		//upload_sessions();
		int do_cloud(void);
		do_cloud(); 
		
		struct treeview_faff tvfaff;
		HWND hwndSess = GetDlgItem(hConfigWnd, IDCX_SESSIONTREEVIEW);
		tvfaff.treeview = hwndSess;
		memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
		refresh_session_treeview(hwndSess, &tvfaff, "");
	}
}
void import(union control *ctrl, void *dlg,
			  void *data, int event)
{
	if (event == EVENT_ACTION) {
		//extern HWND hConfigWnd;
		//HWND hwndSess = GetDlgItem(hConfigWnd, IDCX_SESSIONTREEVIEW);
 		//restore_session_treeview(hwndSess, NULL,  "", SESSION_NONE);

		//extern void download_sessions();
		//download_sessions();
		int do_cloud(void);
		do_cloud();

		struct treeview_faff tvfaff;
		HWND hwndSess = GetDlgItem(hConfigWnd, IDCX_SESSIONTREEVIEW);
		tvfaff.treeview = hwndSess;
		memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
		refresh_session_treeview(hwndSess, &tvfaff, "");
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
    HWND hw, sessionview, treeview;
    struct treeview_faff tvfaff;
    int ret;

    switch (msg) {
      case WM_INITDIALOG:
		memset(pre_session, 0, sizeof pre_session);
		dp.hwnd = hwnd;
	  	dragging = false;

    /*
	 * Centre the window.
	 */
	{			       /* centre the window */
	    RECT rs, rd;

	    //hw = GetDesktopWindow();
		//if (GetWindowRect(hw, &rs) && GetWindowRect(hwnd, &rd)){
		rs = getMaxWorkArea();
	    if (GetWindowRect(hwnd, &rd)){
            if (showSessionTreeview) 
                rd.right += 100 + SESSION_TREEVIEW_WIDTH;
    		MoveWindow(hwnd,
			   (rs.right + rs.left + rd.left - rd.right) / 2,
			   (rs.bottom + rs.top + rd.top - rd.bottom) / 2,
			   rd.right - rd.left, rd.bottom - rd.top, TRUE);
	    }
	}
	create_controls(hwnd, "");     /* Open and Cancel buttons etc */
	SetWindowText(hwnd, dp.wintitle);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        if (has_help())
            SetWindowLongPtr(hwnd, GWL_EXSTYLE,
			     GetWindowLongPtr(hwnd, GWL_EXSTYLE) |
			     WS_EX_CONTEXTHELP);
        else {
            HWND item = GetDlgItem(hwnd, IDC_HELPBTN);
            if (item)
                DestroyWindow(item);
        }
	SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG,
		    (LPARAM) LoadIcon(hinst, MAKEINTRESOURCE(IDI_CFGICON)));

    if (showSessionTreeview){
        /*
    	 * Create the session tree view.
    	 */
    	sessionview = create_session_treeview(hwnd, &tvfaff);
		edit_session_treeview(sessionview, EDIT_INIT);

        /*
    	 * Set up the session view contents.
    	 */
        refresh_session_treeview(sessionview, &tvfaff, DEFAULT_SESSION_NAME);
    }
    
	/*
	 * Create the tree view.
	 */
	{
	    RECT r;
	    WPARAM font;
	    HWND tvstatic;

	    r.left = showSessionTreeview?(SESSION_TREEVIEW_WIDTH+3):3;
	    r.right = r.left + 95;
	    r.top = 3;
	    r.bottom = r.top + 10;
	    MapDialogRect(hwnd, &r);
	    tvstatic = CreateWindowEx(0, "STATIC", "Cate&gory:",
				      WS_CHILD | WS_VISIBLE,
				      r.left, r.top,
				      r.right - r.left, r.bottom - r.top,
				      hwnd, (HMENU) IDCX_TVSTATIC, hinst,
				      NULL);
	    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
	    SendMessage(tvstatic, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

	    r.left = showSessionTreeview?(SESSION_TREEVIEW_WIDTH+3):3;
	    r.right = r.left + 95;
	    r.top = 13;
	    r.bottom = r.top + TREEVIEW_HEIGHT;
	    MapDialogRect(hwnd, &r);
	    treeview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
				      WS_CHILD | WS_VISIBLE |
				      WS_TABSTOP | TVS_HASLINES |
				      TVS_DISABLEDRAGDROP | TVS_HASBUTTONS
				      | TVS_LINESATROOT |
				      TVS_SHOWSELALWAYS, r.left, r.top,
				      r.right - r.left, r.bottom - r.top,
				      hwnd, (HMENU) IDCX_TREEVIEW, hinst,
				      NULL);
	    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
	    SendMessage(treeview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));
	    tvfaff.treeview = treeview;
	    memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
	}

	/*
	 * Set up the tree view contents.
	 */

	refresh_tree_view(ANDROID_DIR_FOLDER_NAME, DEFAULT_SESSION_NAME, tvfaff.treeview);

	/*
	 * Set focus into the first available control.
	 */
	if (showSessionTreeview){
		SetFocus(GetDlgItem(hwnd,IDCX_SEARCHBAR));
	}else
	{
	    int i;
	    struct winctrl *c;

	    for (i = 0; (c = winctrl_findbyindex(&ctrls_panel, i)) != NULL;
		 i++) {
		if (c->ctrl) {
		    dlg_set_focus(c->ctrl, &dp);
		    break;
		}
	    }
	}

	SetWindowLongPtr(hwnd, GWLP_USERDATA, 1);
	return 0;
	  case WM_MOUSEMOVE:
			drag_session_treeview(GetDlgItem(hwnd,IDCX_SESSIONTREEVIEW)
				, DRAG_MOVE, wParam, lParam);
			break;
      case WM_LBUTTONUP:
			if(drag_session_treeview(GetDlgItem(hwnd,IDCX_SESSIONTREEVIEW)
				, DRAG_END, wParam, lParam))
				break;
	/*
	 * Button release should trigger WM_OK if there was a
	 * previous double click on the session list.
	 */
	ReleaseCapture();
	if (dp.ended)
	    SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
	break;
      case WM_NOTIFY:
	if (LOWORD(wParam) == IDCX_TREEVIEW &&
	    ((LPNMHDR) lParam)->code == TVN_SELCHANGED &&
		!isFreshingSessionTreeView && !isFreshingTreeView) {
	    HTREEITEM i =
		TreeView_GetSelection(((LPNMHDR) lParam)->hwndFrom);
	    TVITEM item;
	    char buffer[64];
 
 	    //SendMessage (hwnd, WM_SETREDRAW, FALSE, 0);
 
	    item.hItem = i;
	    item.pszText = buffer;
	    item.cchTextMax = sizeof(buffer);
	    item.mask = TVIF_TEXT | TVIF_PARAM;
	    TreeView_GetItem(((LPNMHDR) lParam)->hwndFrom, &item);
	    {
		/* Destroy all controls in the currently visible panel. */
		int k;
		HWND item;
		struct winctrl *c;

		while ((c = winctrl_findbyindex(&ctrls_panel, 0)) != NULL) {
		    for (k = 0; k < c->num_ids; k++) {
			item = GetDlgItem(hwnd, c->base_id + k);
			if (item)
			    DestroyWindow(item);
		    }
		    winctrl_rem_shortcuts(&dp, c);
		    winctrl_remove(&ctrls_panel, c);
		    sfree(c->data);
		    sfree(c);
		}
	    }
	    create_controls(hwnd, (char *)item.lParam);

	    dlg_refresh(NULL, &dp);    /* set up control values */
 
	    //SendMessage (hwnd, WM_SETREDRAW, TRUE, 0);
 	    //InvalidateRect (hwnd, NULL, TRUE);

	    SetFocus(((LPNMHDR) lParam)->hwndFrom);	/* ensure focus stays */
	    return 0;
	}else if (LOWORD(wParam) == IDCX_SESSIONTREEVIEW 
	&& !isFreshingSessionTreeView){
		switch(((LPNMHDR) lParam)->code){
		case TVN_SELCHANGED:
        	change_selected_session(((LPNMHDR) lParam)->hwndFrom);
			break;

		case NM_DBLCLK:
			if ((change_selected_session(((LPNMHDR) lParam)->hwndFrom) == SESSION_ITEM)
					&& conf_launchable(cfg)){
				dlg_end(&dp, 1);
				SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
			}
			break;
		case TVN_KEYDOWN:
            if ((wParam&VK_CONTROL) || (wParam&VK_LCONTROL)
                || (wParam&VK_RCONTROL))
				drag_session_treeview(GetDlgItem(hwnd,IDCX_SESSIONTREEVIEW)
				, DRAG_CTRL_DOWN, wParam, lParam);
			
            break;
			
		case TVN_BEGINLABELEDIT:		
			edit_session_treeview(((LPNMHDR) lParam)->hwndFrom, EDIT_BEGIN);
			break;
            
		case TVN_ENDLABELEDIT:		
			edit_session_treeview(((LPNMHDR) lParam)->hwndFrom, EDIT_END);
			break;

		case NM_RCLICK:
			show_st_popup_menu(((LPNMHDR) lParam)->hwndFrom);
			break;

		case TVN_BEGINDRAG:
			drag_session_treeview(((LPNMHDR) lParam)->hwndFrom
				, DRAG_BEGIN, wParam, lParam);
			break;

        case TVN_ITEMEXPANDED:{
            HWND treeview = GetDlgItem(hwnd,IDCX_SESSIONTREEVIEW);
            char session[256] = {0};
            LPNMTREEVIEW pnmtv = (LPNMTREEVIEW) lParam;
			if (SESSION_GROUP == conv_tv_to_sess(treeview,
				pnmtv->itemNew.hItem, session, sizeof(session)))
			{
				save_isetting(session, GRP_COLLAPSE_SETTING, pnmtv->action == 1);
				if (strcmp(session, pre_session) == 0)
				{
					conf_set_int(cfg, CONF_group_collapse, pnmtv->action == 1);
				}
			}
            break;
        }
		default:
			break;

		};//switch
		return 0;
    }
	else
	{
		return winctrl_handle_command(&dp, msg, wParam, lParam);
	}
	break;
      case WM_COMMAND:
	  	if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDCX_SEARCHBAR){
			struct treeview_faff tvfaff;
			HWND hwndSess = GetDlgItem(hwnd,IDCX_SESSIONTREEVIEW);
			tvfaff.treeview = hwndSess;
			memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
			refresh_session_treeview(hwndSess, &tvfaff, "");
			SetFocus(GetDlgItem(hwnd,IDCX_SEARCHBAR));
			return 0;
	  	}
      case WM_DRAWITEM:
      default:			       /* also handle drag list msg here */
	/*
	 * Only process WM_COMMAND once the dialog is fully formed.
	 */
	if (GetWindowLongPtr(hwnd, GWLP_USERDATA) == 1) {
	    ret = winctrl_handle_command(&dp, msg, wParam, lParam);
	    if (dp.ended && GetCapture() != hwnd)
		SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
	} else
	    ret = 0;
	return ret;
      case WM_HELP:
	if (!winctrl_context_help(&dp, hwnd,
				 ((LPHELPINFO)lParam)->iCtrlId))
	    MessageBeep(0);
        break;
      case WM_CLOSE:
	quit_help(hwnd);
	SaneEndDialog(hwnd, 0);
	return 0;

	/* Grrr Explorer will maximize Dialogs! */
      case WM_SIZE:
	if (wParam == SIZE_MAXIMIZED)
	    force_normal(hwnd);
	return 0;

    }
    return 0;
}

void modal_about_box(HWND hwnd)
{
    EnableWindow(hwnd, 0);
    DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, AboutProc);
    EnableWindow(hwnd, 1);
    SetActiveWindow(hwnd);
}

void show_help(HWND hwnd)
{
    launch_help(hwnd, NULL);
}

void defuse_showwindow(void)
{
    /*
     * Work around the fact that the app's first call to ShowWindow
     * will ignore the default in favour of the shell-provided
     * setting.
     */
    {
	HWND hwnd;
	hwnd = CreateDialog(hinst, MAKEINTRESOURCE(IDD_ABOUTBOX),
			    NULL, NullDlgProc);
	ShowWindow(hwnd, SW_HIDE);
	SetActiveWindow(hwnd);
	DestroyWindow(hwnd);
    }
}

int do_config(void)
{
    int ret;

	if (hConfigWnd){
		bringToForeground(hConfigWnd);
		return 0;
	}

    memset(pre_session, 0, sizeof pre_session);
    showSessionTreeview = 1;
    ctrlbox = ctrl_new_box();
    setup_config_box(ctrlbox, FALSE, 0, 0);
    win_setup_config_box(ctrlbox, &dp.hwnd, has_help(), FALSE, 0);
    dp_init(&dp);
    winctrl_init(&ctrls_base);
    winctrl_init(&ctrls_panel);
    dp_add_tree(&dp, &ctrls_base);
    dp_add_tree(&dp, &ctrls_panel);
    dp.wintitle = dupprintf("%s Configuration", appname);
    dp.errtitle = dupprintf("%s Error", appname);
    dp.data = cfg;
    dlg_auto_set_fixed_pitch_flag(&dp);
    dp.shortcuts['g'] = TRUE;	       /* the treeview: `Cate&gory' */

    ret =
	SaneDialogBox(hinst, MAKEINTRESOURCE(IDD_MAINBOX), NULL,
		  GenericMainDlgProc);
	
    ctrl_free_box(ctrlbox);
    winctrl_cleanup(&ctrls_panel);
    winctrl_cleanup(&ctrls_base);
    dp_cleanup(&dp);
    showSessionTreeview = 0;
    memset(pre_session, 0, sizeof pre_session);
	
    return ret;
}

int do_reconfig(HWND hwnd, int protcfginfo)
{
    Conf* backup_cfg;
    int ret;

	if (hConfigWnd){
		bringToForeground(hConfigWnd);
		return 0;
	}

    backup_cfg = conf_copy( cfg);		       /* structure copy */

    memset(pre_session, 0, sizeof pre_session);
    ctrlbox = ctrl_new_box();
	int protocol = conf_get_int(cfg, CONF_protocol);
    setup_config_box(ctrlbox, TRUE, protocol, protcfginfo);
    win_setup_config_box(ctrlbox, &dp.hwnd, has_help(), TRUE,
                         protocol);
    dp_init(&dp);
    winctrl_init(&ctrls_base);
    winctrl_init(&ctrls_panel);
    dp_add_tree(&dp, &ctrls_base);
    dp_add_tree(&dp, &ctrls_panel);
    dp.wintitle = dupprintf("%s Reconfiguration", appname);
    dp.errtitle = dupprintf("%s Error", appname);
    dp.data = cfg;
    dlg_auto_set_fixed_pitch_flag(&dp);
    dp.shortcuts['g'] = TRUE;	       /* the treeview: `Cate&gory' */

    ret = SaneDialogBox(hinst, MAKEINTRESOURCE(IDD_MAINBOX), NULL,
		  GenericMainDlgProc);

    ctrl_free_box(ctrlbox);
    winctrl_cleanup(&ctrls_base);
    winctrl_cleanup(&ctrls_panel);
    dp_cleanup(&dp);

    if (!ret)
	{
		conf_copy_into(cfg, backup_cfg);	       /* structure copy */
	}
	conf_free(backup_cfg);

    memset(pre_session, 0, sizeof pre_session);
	extern void reinit_shortcut_rules();
	reinit_shortcut_rules();
    return ret;
}

const char* show_input_dialog(const char* const caption, const char* tips, char* origin)
{
	strncpy(InputBoxCaption, caption, sizeof(InputBoxCaption));
	strncpy(InputBoxTips, tips, sizeof(InputBoxTips));
	strncpy(InputBoxStr, origin, sizeof(InputBoxTips));

	int ret = DialogBox(hinst, MAKEINTRESOURCE(IDD_INPUT_BOX), hConfigWnd != NULL ? hConfigWnd : hTopWnd, InputDialogProc);
	if (ret == IDOK)
	{
		return InputBoxStr;
	}
	return NULL;
}

void showabout(HWND hwnd)
{
    DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, AboutProc);
	//show_input_dialog("Change Tab Title", "please enter the new title");
}


int verify_ssh_host_key(void *frontend, char *host, int port,
                        const char *keytype, char *keystr, char *fingerprint,
                        void (*callback)(void *ctx, int result), void *ctx)
{
    int ret;

    static const char absentmsg[] =
	"The server's host key is not cached in the registry. You\n"
	"have no guarantee that the server is the computer you\n"
	"think it is.\n"
	"The server's %s key fingerprint is:\n"
	"%s\n"
	"If you trust this host, hit Yes to add the key to\n"
	"%s's cache and carry on connecting.\n"
	"If you want to carry on connecting just once, without\n"
	"adding the key to the cache, hit No.\n"
	"If you do not trust this host, hit Cancel to abandon the\n"
	"connection.\n";

    static const char wrongmsg[] =
	"WARNING - POTENTIAL SECURITY BREACH!\n"
	"\n"
	"The server's host key does not match the one %s has\n"
	"cached in the registry. This means that either the\n"
	"server administrator has changed the host key, or you\n"
	"have actually connected to another computer pretending\n"
	"to be the server.\n"
	"The new %s key fingerprint is:\n"
	"%s\n"
	"If you were expecting this change and trust the new key,\n"
	"hit Yes to update %s's cache and continue connecting.\n"
	"If you want to carry on connecting but without updating\n"
	"the cache, hit No.\n"
	"If you want to abandon the connection completely, hit\n"
	"Cancel. Hitting Cancel is the ONLY guaranteed safe\n" "choice.\n";

    static const char mbtitle[] = "%s Security Alert";

    /*
     * Verify the key against the registry.
     */
    ret = gStorage->verify_host_key(host, port, keytype, keystr);

    if (ret == 0)		       /* success - key matched OK */
	return 1;
    else if (ret == 2) {	       /* key was different */
	int mbret;
	char *text = dupprintf(wrongmsg, appname, keytype, fingerprint,
			       appname);
	char *caption = dupprintf(mbtitle, appname);
	mbret = message_box(text, caption,
			    MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3,
			    HELPCTXID(errors_hostkey_changed));
	assert(mbret==IDYES || mbret==IDNO || mbret==IDCANCEL);
	sfree(text);
	sfree(caption);
	if (mbret == IDYES) {
	    gStorage->store_host_key(host, port, keytype, keystr);
	    return 1;
	} else if (mbret == IDNO)
	    return 1;
    } else if (ret == 1) {	       /* key was absent */
	int mbret;
	char *text = dupprintf(absentmsg, keytype, fingerprint, appname);
	char *caption = dupprintf(mbtitle, appname);
	mbret = message_box(text, caption,
			    MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3,
			    HELPCTXID(errors_hostkey_absent));
	assert(mbret==IDYES || mbret==IDNO || mbret==IDCANCEL);
	sfree(text);
	sfree(caption);
	if (mbret == IDYES) {
	    gStorage->store_host_key(host, port, keytype, keystr);
	    return 1;
	} else if (mbret == IDNO)
	    return 1;
    }
    return 0;	/* abandon the connection */
}

/*
 * Ask whether the selected algorithm is acceptable (since it was
 * below the configured 'warn' threshold).
 */
int askalg(void *frontend, const char *algtype, const char *algname,
	   void (*callback)(void *ctx, int result), void *ctx)
{
    static const char mbtitle[] = "%s Security Alert";
    static const char msg[] =
	"The first %s supported by the server\n"
	"is %.64s, which is below the configured\n"
	"warning threshold.\n"
	"Do you want to continue with this connection?\n";
    char *message, *title;
    int mbret;

    message = dupprintf(msg, algtype, algname);
    title = dupprintf(mbtitle, appname);
    mbret = MessageBox(NULL, message, title,
		MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2 | MB_TOPMOST);
    socket_reselect_all();
    sfree(message);
    sfree(title);
    if (mbret == IDYES)
	return 1;
    else
	return 0;
}

/*
 * Ask whether to wipe a session log file before writing to it.
 * Returns 2 for wipe, 1 for append, 0 for cancel (don't log).
 */
int askappend(void *frontend, Filename *filename,
	      void (*callback)(void *ctx, int result), void *ctx)
{
    static const char msgtemplate[] =
	"The session log file \"%.*s\" already exists.\n"
	"You can overwrite it with a new session log,\n"
	"append your session log to the end of it,\n"
	"or disable session logging for this session.\n"
	"Hit Yes to wipe the file, No to append to it,\n"
	"or Cancel to disable logging.";
    char *message;
    char *mbtitle;
    int mbret;

    message = dupprintf(msgtemplate, FILENAME_MAX, filename->path);
    mbtitle = dupprintf("%s Log to File", appname);

    mbret = MessageBox(NULL, message, mbtitle,
		MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON3 | MB_TOPMOST);

    socket_reselect_all();

    sfree(message);
    sfree(mbtitle);

    if (mbret == IDYES)
	return 2;
    else if (mbret == IDNO)
	return 1;
    else
	return 0;
}

/*
 * Warn about the obsolescent key file format.
 * 
 * Uniquely among these functions, this one does _not_ expect a
 * frontend handle. This means that if PuTTY is ported to a
 * platform which requires frontend handles, this function will be
 * an anomaly. Fortunately, the problem it addresses will not have
 * been present on that platform, so it can plausibly be
 * implemented as an empty function.
 */
void old_keyfile_warning(void)
{
    static const char mbtitle[] = "%s Key File Warning";
    static const char message[] =
	"You are loading an SSH-2 private key which has an\n"
	"old version of the file format. This means your key\n"
	"file is not fully tamperproof. Future versions of\n"
	"%s may stop supporting this private key format,\n"
	"so we recommend you convert your key to the new\n"
	"format.\n"
	"\n"
	"You can perform this conversion by loading the key\n"
	"into PuTTYgen and then saving it again.";

    char *msg, *title;
    msg = dupprintf(message, appname);
    title = dupprintf(mbtitle, appname);

	MessageBox(NULL, msg, title, MB_OK | MB_TOPMOST);

    socket_reselect_all();

    sfree(msg);
    sfree(title);
}
