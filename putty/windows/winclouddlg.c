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

static bool isFreshingSessionTreeView = false;
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
	IDCX_CLOUD = IDC_CLOUD,
    IDCX_TVSTATIC,
    IDCX_SEARCHBAR,
    IDCX_SESSIONTREEVIEW,
    IDCX_CLOUDTREEVIEW,
    IDCX_STDBASE,
    IDCX_PANELBASE = IDCX_STDBASE + 32
};

struct treeview_faff {
    HWND treeview;
    HTREEITEM lastat[128];
};

extern const BYTE ANDmaskCursor[] ; 
extern const BYTE XORmaskCursor[];

static HCURSOR hCopyCurs = NULL; 
static const int SESSION_TREEVIEW_WIDTH = 160;
static const int TREEVIEW_HEIGHT = 269;
static RECT dlgMonitorRect;

extern Conf* cfg;		       /* defined in window.c */

#define GRP_COLLAPSE_SETTING "GroupCollapse"

static void refresh_session_treeview(
	HWND sessionview,
	struct treeview_faff* tvfaff,
	const char* select_session);
RECT getMaxWorkArea();
LPARAM get_selected_session(HWND hwndSess, char* const sess_name, const int name_len);

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
    wc.lpszClassName = "PuTTY-ND2_CloudSessions";
    RegisterClassA(&wc);

    hwnd = CreateDialog(hinst, tmpl, hwndparent, lpDialogFunc);
	if (hwnd == NULL){
		ErrorExit("PuTTY-ND2_CloudSessions");
		return -1;
	}
	extern HWND hCloudWnd;
	hCloudWnd = hwnd;

	ShowWindow(hwnd, SW_HIDE);
	ShowWindow(hwnd, SW_SHOW);

	bringToForeground(hwnd);
    SetWindowLongPtr(hwnd, BOXFLAGS, 0); /* flags */
    SetWindowLongPtr(hwnd, BOXRESULT, 0); /* result from SaneEndDialog */

    while ((gm=GetMessage(&msg, NULL, 0, 0)) > 0) {
    	if(msg.message == WM_KEYUP){
    	}else if (msg.message == WM_KEYDOWN){
			if (msg.wParam == VK_CONTROL){
			}
			else if (msg.wParam == VK_F2)
			{
				HWND hwndSess = GetDlgItem(hwnd, IDCX_SESSIONTREEVIEW);
				TreeView_EditLabel(hwndSess, TreeView_GetSelection(hwndSess));
			    continue;
			}
            if (msg.wParam == VK_ESCAPE){
				if (ctrlbox->cancelbutton != NULL){
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
			if (msg.wParam == VK_DOWN){
				SetFocus(GetDlgItem(hwnd,IDCX_SESSIONTREEVIEW));
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
	hCloudWnd = NULL;
	SetActiveWindow(hwndparent);
    return ret;
}

extern HTREEITEM session_treeview_insert(struct treeview_faff *faff,
	int level, char *text, LPARAM flags);
extern const char* extract_last_part(const char* session);
extern void extract_group(const char* session,
	char* group, const int glen);
extern LPARAM conv_tv_to_sess(
	HWND hwndSess, HTREEITEM hitem,
	char* const sess_name, const int name_len);
extern LPARAM get_selected_session(HWND hwndSess, char* const sess_name, const int name_len);


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

    r.left = 3;
    r.right = r.left + SESSION_TREEVIEW_WIDTH - 6;
    r.top = 3;
    r.bottom = r.top + 10;
    MapDialogRect(hwnd, &r);
	const int SEARCH_TEXT_LEN = SESSION_TREEVIEW_WIDTH;
    tvstatic = CreateWindowEx(0, "STATIC", "Local Sessions",
			      WS_CHILD | WS_VISIBLE,
			      r.left, r.top,
			      SEARCH_TEXT_LEN, r.bottom - r.top,
			      hwnd, (HMENU) IDCX_TVSTATIC, hinst,
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

	hImageList = ImageList_Create(16,16,ILC_COLOR16,3,10);
	hBitMap = LoadBitmap(hinst,MAKEINTRESOURCE(IDB_TREE));
	ImageList_Add(hImageList,hBitMap,NULL);
	DeleteObject(hBitMap);
	SendDlgItemMessage(hwnd, IDCX_SESSIONTREEVIEW, TVM_SETIMAGELIST,0,(LPARAM)hImageList); 

	return sessionview;
}

static HWND create_cloud_treeview(HWND hwnd, struct treeview_faff* tvfaff)
{
	RECT r;
	WPARAM font;
	HWND tvstatic;
	HWND searchbar;
	HWND sessionview;
	HIMAGELIST hImageList;
	HBITMAP hBitMap;
	int i;
	RECT rd;
	GetWindowRect(hwnd, &rd);

	r.left = SESSION_TREEVIEW_WIDTH + 50 + 3;
	r.right = r.left + SESSION_TREEVIEW_WIDTH - 6;
	r.top = 3;
	r.bottom = r.top + 10;
	MapDialogRect(hwnd, &r);
	const int SEARCH_TEXT_LEN = SESSION_TREEVIEW_WIDTH;
	tvstatic = CreateWindowEx(0, "STATIC", "Remote Sessions",
		WS_CHILD | WS_VISIBLE,
		r.left, r.top,
		SEARCH_TEXT_LEN, r.bottom - r.top,
		hwnd, (HMENU)IDCX_TVSTATIC, hinst,
		NULL);
	font = SendMessage(hwnd, WM_GETFONT, 0, 0);
	SendMessage(tvstatic, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

	r.left = SESSION_TREEVIEW_WIDTH + 50 + 3;
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
		hwnd, (HMENU)IDCX_CLOUDTREEVIEW, hinst,
		NULL);
	font = SendMessage(hwnd, WM_GETFONT, 0, 0);
	SendMessage(sessionview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));
	tvfaff->treeview = sessionview;
	memset(tvfaff->lastat, 0, sizeof(tvfaff->lastat));

	hImageList = ImageList_Create(16, 16, ILC_COLOR16, 3, 10);
	hBitMap = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_TREE));
	ImageList_Add(hImageList, hBitMap, NULL);
	DeleteObject(hBitMap);
	SendDlgItemMessage(hwnd, IDCX_CLOUDTREEVIEW, TVM_SETIMAGELIST, 0, (LPARAM)hImageList);

	return sessionview;
}


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

	isFreshingSessionTreeView = true;
    memset(tvfaff->lastat, 0, sizeof(tvfaff->lastat));
	TreeView_DeleteAllItems(tvfaff->treeview);

    get_sesslist(&sesslist, TRUE);	
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
	}else{
		dlg_refresh(NULL, &dp);
	}
	get_sesslist(&sesslist, FALSE);
}

/*
 * copy session, return FALSE if to_session exist
 */
extern int copy_session(
	struct sesslist* sesslist,
	const char* from_session,
	const char* to_session,
	int to_sess_flag);

extern int copy_item_under_group(const char* session_name, const char* origin_group, const char* dest_group);
extern void dup_session_treeview(
	HWND hwndSess,
	HTREEITEM selected_item,
	const char* from_session,
	const char* to_session_pre,
	int from_sess_flag,
	int to_sess_flag);

/*
 * This function is the configuration box.
 * (Being a dialog procedure, in general it returns 0 if the default
 * dialog processing should be performed, and 1 if it should not.)
 */
static int CALLBACK GenericMainDlgProc(HWND hwnd, UINT msg,
				       WPARAM wParam, LPARAM lParam)
{
    HWND hw, sessionview, cloudtreeview;
    struct treeview_faff tvfaff;
    int ret;

    switch (msg) {
      case WM_INITDIALOG:
		dp.hwnd = hwnd;

    /*
	 * Centre the window.
	 */
	{			       /* centre the window */
	    RECT rs, rd;

	    //hw = GetDesktopWindow();
		//if (GetWindowRect(hw, &rs) && GetWindowRect(hwnd, &rd)){
		rs = getMaxWorkArea();
	    if (GetWindowRect(hwnd, &rd)){
            rd.right += 100 + SESSION_TREEVIEW_WIDTH;
    		MoveWindow(hwnd,
			   (rs.right + rs.left + rd.left - rd.right) / 2,
			   (rs.bottom + rs.top + rd.top - rd.bottom) / 2,
			   rd.right - rd.left, rd.bottom - rd.top, TRUE);
	    }
	}
	//create_controls(hwnd, "");     /* Open and Cancel buttons etc */
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
    /*
    	* Create the session tree view.
    	*/
    sessionview = create_session_treeview(hwnd, &tvfaff);
    refresh_session_treeview(sessionview, &tvfaff, DEFAULT_SESSION_NAME);
	
	cloudtreeview = create_cloud_treeview(hwnd, &tvfaff);

	SetWindowLongPtr(hwnd, GWLP_USERDATA, 1);
	return 0;
	/*
	 * Button release should trigger WM_OK if there was a
	 * previous double click on the session list.
	 */
	ReleaseCapture();
	if (dp.ended)
	    SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
	break;
      case WM_NOTIFY:
	if (LOWORD(wParam) == IDCX_SESSIONTREEVIEW 
	&& !isFreshingSessionTreeView){
		switch(((LPNMHDR) lParam)->code){
		case TVN_SELCHANGED:
			break;

		case NM_DBLCLK:
			break;
		case TVN_KEYDOWN:
            break;
			
		case TVN_BEGINLABELEDIT:
			break;
            
		case TVN_ENDLABELEDIT:		
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
	else
	{
		return winctrl_handle_command(&dp, msg, wParam, lParam);
	}
	break;
      case WM_COMMAND:
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

int do_cloud(void)
{
    int ret;

	if (hCloudWnd){
		bringToForeground(hCloudWnd);
		return 0;
	}

    ctrlbox = ctrl_new_box();
    //setup_config_box(ctrlbox, FALSE, 0, 0);
    //win_setup_config_box(ctrlbox, &dp.hwnd, has_help(), FALSE, 0);
    dp_init(&dp);
    winctrl_init(&ctrls_base);
    winctrl_init(&ctrls_panel);
    dp_add_tree(&dp, &ctrls_base);
    dp_add_tree(&dp, &ctrls_panel);
    dp.wintitle = dupprintf("%s Cloud", appname);
    dp.errtitle = dupprintf("%s Error", appname);
    dp.data = cfg;
    dlg_auto_set_fixed_pitch_flag(&dp);
    dp.shortcuts['g'] = TRUE;	       /* the treeview: `Cate&gory' */

    ret = SaneDialogBox(hinst, MAKEINTRESOURCE(IDD_CLOUD_SESSION_BOX), NULL,
		  GenericMainDlgProc);
	
    ctrl_free_box(ctrlbox);
	ctrlbox = NULL;
    winctrl_cleanup(&ctrls_panel);
    winctrl_cleanup(&ctrls_base);
    dp_cleanup(&dp);
	
    return ret;
}
