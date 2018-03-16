#define PUTTY_DO_GLOBALS
#include "native_putty_common.h"
#include "native_putty_controller.h"
#include "window_interface.h"
#include "terminal.h"
#include "Mmsystem.h"
#include "storage.h"
#include "atlconv.h" 
#include <fstream>
#include <map>
#include <string.h>
#include <base/file_util.h>
#include "putty_view.h"
#include "base/timer.h"
#include "WinWorker.h"
#include "WinProcessor.h"

static wchar_t *clipboard_contents;
static size_t clipboard_length;
Conf* cfg = NULL;
void init_flashwindow();
void pageant_init();
//void fini_local_agent();
base::RepeatingTimer<Processor::WinWorker>  gUITimer;

void process_init()
{
	USES_CONVERSION;

	WindowInterface::GetInstance()->init_ui_msg_loop();
	cfg = conf_new();
	sk_init();
	init_flashwindow();
	pageant_init();

	if (!init_winver())
    {
		char *str = dupprintf("%s Fatal Error", appname);
		MessageBoxA(WindowInterface::GetInstance()->getNativeTopWnd(), "Windows refuses to report a version",
			str, MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST);
		sfree(str);
		exit(-1);
    }
	
	OSVERSIONINFO& osVersion = get_os_version();
    /*
     * If we're running a version of Windows that doesn't support
     * WM_MOUSEWHEEL, find out what message number we should be
     * using instead.
     */
    if (osVersion.dwMajorVersion < 4 ||
	(osVersion.dwMajorVersion == 4 && 
	 osVersion.dwPlatformId != VER_PLATFORM_WIN32_NT))
		NativePuttyController::wm_mousewheel = RegisterWindowMessage(TEXT("MSWHEEL_ROLLMSG"));
	/*
     * If we're running a version of Windows that doesn't support
     * WM_MOUSEWHEEL, find out what message number we should be
     * using instead.
     */
    if (osVersion.dwMajorVersion < 4 ||
	(osVersion.dwMajorVersion == 4 && 
	 osVersion.dwPlatformId != VER_PLATFORM_WIN32_NT)){
		NativePuttyController::wm_mousewheel = RegisterWindowMessageA("MSWHEEL_ROLLMSG");
	}

	default_protocol = be_default_protocol;
	/* Find the appropriate default port. */
	{
	    Backend *b = backend_from_proto(default_protocol);
	    default_port = 0; /* illegal */
	    if (b)
		default_port = b->default_port;
	}
	conf_set_int(cfg, CONF_logtype, LGTYP_NONE);
	do_defaults(NULL, cfg);
	if (conf_get_int(cfg, CONF_data_version) == 1)
	{
		extern void translate_all_session_data();
		translate_all_session_data();
	}

	//popup_menus[SYSMENU].menu = GetSystemMenu(hwnd, FALSE);
	NativePuttyController::popup_menu = CreatePopupMenu();
	AppendMenu(NativePuttyController::popup_menu, MF_ENABLED, IDM_PASTE, TEXT("&Paste"));
	//for (int j = 0; j < lenof(popup_menus); j++) {
	    HMENU m = NativePuttyController::popup_menu;

        AppendMenu(m, MF_SEPARATOR, 0, 0);
        AppendMenu(m, MF_ENABLED | MF_UNCHECKED, IDM_START_STOP_LOG, TEXT("&Start Logging"));
	    AppendMenu(m, MF_SEPARATOR, 0, 0);
	    AppendMenu(m, MF_ENABLED, IDM_SHOWLOG, TEXT("&Event Log"));
	    AppendMenu(m, MF_SEPARATOR, 0, 0);
	    AppendMenu(m, MF_ENABLED, IDM_NEWSESS, TEXT("Ne&w Session..."));
	    AppendMenu(m, MF_ENABLED, IDM_DUPSESS, TEXT("&Duplicate Session"));
	    AppendMenu(m, MF_ENABLED, IDM_RESTART, TEXT("Restart Sessions"));
	    AppendMenu(m, MF_ENABLED, IDM_RECONF, TEXT("Chan&ge Settings..."));
	    AppendMenu(m, MF_SEPARATOR, 0, 0);
	    AppendMenu(m, MF_ENABLED, IDM_COPYALL, TEXT("C&opy All to Clipboard"));
	    AppendMenu(m, MF_ENABLED, IDM_CLRSB, TEXT("C&lear Scrollback"));
	    AppendMenu(m, MF_ENABLED, IDM_RESET, TEXT("Rese&t Terminal"));
	    //AppendMenu(m, MF_SEPARATOR, 0, 0);
	    //AppendMenu(m, (cfg.resize_action == RESIZE_DISABLED) ?
		//       MF_GRAYED : MF_ENABLED, IDM_FULLSCREEN, "&Full Screen");
	    //AppendMenu(m, MF_SEPARATOR, 0, 0);
	    //if (has_help())
		//AppendMenu(m, MF_ENABLED, IDM_HELP, TEXT("&Help"));
	    //char* str = dupprintf("&About %s", appname);
	    //AppendMenu(m, MF_ENABLED, IDM_ABOUT, A2W(str));
	    //sfree(str);
	//}
    
		gUITimer.Start(base::TimeDelta::FromMilliseconds(100), g_ui_processor, &Processor::WinWorker::process_ui_jobs);
}

void process_fini()
{
	//fini_local_agent();
	sk_cleanup();
	gUITimer.Stop();
}
//------------------------------------------------------------------
//for term
Context get_ctx(void *frontend)
{
    assert(frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    if (puttyController->getNativePage()){
    	puttyController->hdc = GetDC(puttyController->getNativePage());
    	if (puttyController->hdc && puttyController->pal){
    	    SelectPalette(puttyController->hdc, puttyController->pal, FALSE);
        }         
    }
    return (Context)puttyController;
}

void free_ctx(void *frontend, Context ctx)
{
    assert(frontend != NULL && ctx != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    if (puttyController->hdc){
        SelectPalette(puttyController->hdc, (HPALETTE__*)GetStockObject(DEFAULT_PALETTE), FALSE);
        ReleaseDC(puttyController->getNativePage(), puttyController->hdc);
        puttyController->hdc = NULL;
    }
}

/*
 * Move the system caret. (We maintain one, even though it's
 * invisible, for the benefit of blind people: apparently some
 * helper software tracks the system caret, so we should arrange to
 * have one.)
 */
void sys_cursor(void *frontend, int x, int y)
{
    assert(frontend != NULL);
    int cx, cy;

    NativePuttyController *puttyController = (NativePuttyController *)frontend;

    if (!puttyController->term->has_focus) return;

    /*
     * Avoid gratuitously re-updating the cursor position and IMM
     * window if there's no actual change required.
     */
    cx = x * puttyController->font_width + puttyController->offset_width;
    cy = y * puttyController->font_height + puttyController->offset_height;
    if (cx == puttyController->caret_x && cy == puttyController->caret_y)
	return;
    puttyController->caret_x = cx;
    puttyController->caret_y = cy;

    puttyController->sys_cursor_update();
}

void do_cursor(Context ctx, int x, int y, wchar_t *text, int len,
	       unsigned long attr, int lattr)
{
    assert(ctx != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)ctx;
    int fnt_width;
    int char_width;
    HDC hdc = puttyController->hdc;
    int ctype = conf_get_int( puttyController->cfg, CONF_cursor_type);

    assert (hdc != NULL);

    lattr &= LATTR_MODE;

    if ((attr & TATTR_ACTCURS) && (ctype == 0 || puttyController->term->big_cursor)) {
	if (*text != UCSWIDE) {
	    do_text(ctx, x, y, text, len, attr, lattr);
	    return;
	}
	ctype = 2;
	attr |= TATTR_RIGHTCURS;
    }

    fnt_width = char_width = puttyController->font_width * (1 + (lattr != LATTR_NORM));
    if (attr & ATTR_WIDE)
	char_width *= 2;
    x *= fnt_width;
    y *= puttyController->font_height;
    x += puttyController->offset_width;
    y += puttyController->offset_height;

    if ((attr & TATTR_PASCURS) && (ctype == 0 || puttyController->term->big_cursor)) {
	POINT pts[5];
	HPEN oldpen;
	pts[0].x = pts[1].x = pts[4].x = x;
	pts[2].x = pts[3].x = x + char_width - 1;
	pts[0].y = pts[3].y = pts[4].y = y;
	pts[1].y = pts[2].y = y + puttyController->font_height - 1;
	oldpen = (HPEN__*)SelectObject(hdc, CreatePen(PS_SOLID, 0, puttyController->colours[261]));
	Polyline(hdc, pts, 5);
	oldpen = (HPEN__*)SelectObject(hdc, oldpen);
	DeleteObject(oldpen);
    } else if ((attr & (TATTR_ACTCURS | TATTR_PASCURS)) && ctype != 0) {
	int startx, starty, dx, dy, length, i;
	if (ctype == 1) {
	    startx = x;
	    starty = y + puttyController->descent;
	    dx = 1;
	    dy = 0;
	    length = char_width;
	} else {
	    int xadjust = 0;
	    if (attr & TATTR_RIGHTCURS)
		xadjust = char_width - 1;
	    startx = x + xadjust;
	    starty = y;
	    dx = 0;
	    dy = 1;
	    length = puttyController->font_height;
	}
	if (attr & TATTR_ACTCURS) {
	    HPEN oldpen;
	    oldpen =
	    		(HPEN__*)SelectObject(hdc, CreatePen(PS_SOLID, 0, puttyController->colours[261]));
	    MoveToEx(hdc, startx, starty, NULL);
	    LineTo(hdc, startx + dx * length, starty + dy * length);
	    oldpen = (HPEN__*)SelectObject(hdc, oldpen);
	    DeleteObject(oldpen);
	} else {
	    for (i = 0; i < length; i++) {
		if (i % 2 == 0) {
		    SetPixel(hdc, startx, starty, puttyController->colours[261]);
		}
		startx += dx;
		starty += dy;
	    }
	}
    }
}

/*
 * set or clear the "raw mouse message" mode
 */
void set_raw_mouse_mode(void *frontend, int activate)
{
    assert (frontend != NULL);
	NativePuttyController *puttyController = (NativePuttyController *)frontend;
    activate = activate && !conf_get_int( puttyController->cfg, CONF_no_mouse_rep);
    puttyController->send_raw_mouse = activate;
    puttyController->update_mouse_pointer();
}

void set_sbar(void *frontend, int total, int start, int page)
{
    assert (frontend != NULL);

    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    SCROLLINFO si;

    //if (is_full_screen() ? !puttyController->cfg->scrollbar_in_fullscreen : !puttyController->cfg->scrollbar)
	//return;
	if (!conf_get_int( puttyController->cfg, CONF_scrollbar))
		return ;

    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    si.nMin = 0;
    si.nMax = total - 1;
    si.nPage = page;
    si.nPos = start;
    if (puttyController->getNativePage())
    	SetScrollInfo(puttyController->getNativePage(), SB_VERT, &si, TRUE);
}

void fatalbox(const char *fmt, ...)
{
	USES_CONVERSION;
    va_list ap;
    char *stuff, morestuff[100];

    va_start(ap, fmt);
    stuff = dupvprintf(fmt, ap);
    va_end(ap);
    sprintf(morestuff, "%.70s Fatal Error", appname);
	MessageBox(WindowInterface::GetInstance()->getNativeTopWnd(), A2W(stuff), A2W(morestuff), MB_ICONERROR | MB_OK | MB_TOPMOST);
    sfree(stuff);
    cleanup_exit(1);
}
/*
 * Wrapper that handles combining characters.
 */
void do_text(Context ctx, int x, int y, wchar_t *text, int len,
	     unsigned long attr, int lattr)
{
    assert(ctx != NULL);

    NativePuttyController *puttyController = (NativePuttyController *)ctx;
    if (attr & TATTR_COMBINING) {
	unsigned long a = 0;
	attr &= ~TATTR_COMBINING;
	while (len--) {
	    puttyController->do_text_internal(x, y, text, 1, attr | a, lattr);
	    text++;
	    a = TATTR_COMBINING;
	}
    } else
	puttyController->do_text_internal(x, y, text, len, attr, lattr);
}

/* This function gets the actual width of a character in the normal font.
 */
int char_width(Context ctx, int uc) {
    assert(ctx != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)ctx;
    HDC hdc = puttyController->hdc;
    int ibuf = 0;

    assert (hdc != NULL);

    /* If the font max is the same as the font ave width then this
     * function is a no-op.
     */
    if (!puttyController->font_dualwidth) return 1;

    switch (uc & CSET_MASK) {
      case CSET_ASCII:
	uc = puttyController->ucsdata.unitab_line[uc & 0xFF];
	break;
      case CSET_LINEDRW:
	uc = puttyController->ucsdata.unitab_xterm[uc & 0xFF];
	break;
      case CSET_SCOACS:
	uc = puttyController->ucsdata.unitab_scoacs[uc & 0xFF];
	break;
    }
    if (DIRECT_FONT(uc)) {
	if (puttyController->ucsdata.dbcs_screenfont) return 1;

	/* Speedup, I know of no font where ascii is the wrong width */
	if ((uc&~CSET_MASK) >= ' ' && (uc&~CSET_MASK)<= '~')
	    return 1;

	if ( (uc & CSET_MASK) == CSET_ACP ) {
	    SelectObject(hdc, puttyController->fonts[FONT_NORMAL]);
	} else if ( (uc & CSET_MASK) == CSET_OEMCP ) {
	    puttyController->another_font(FONT_OEM);
	    if (!puttyController->fonts[FONT_OEM]) return 0;

	    SelectObject(hdc, puttyController->fonts[FONT_OEM]);
	} else
	    return 0;

	if ( GetCharWidth32(hdc, uc&~CSET_MASK, uc&~CSET_MASK, &ibuf) != 1 &&
	     GetCharWidth(hdc, uc&~CSET_MASK, uc&~CSET_MASK, &ibuf) != 1)
	    return 0;
    } else {
	/* Speedup, I know of no font where ascii is the wrong width */
	if (uc >= ' ' && uc <= '~') return 1;

	SelectObject(hdc, puttyController->fonts[FONT_NORMAL]);
	if ( GetCharWidth32W(hdc, uc, uc, &ibuf) == 1 )
	    /* Okay that one worked */ ;
	else if ( GetCharWidthW(hdc, uc, uc, &ibuf) == 1 )
	    /* This should work on 9x too, but it's "less accurate" */ ;
	else
	    return 0;
    }

    ibuf += puttyController->font_width / 2 -1;
    ibuf /= puttyController->font_width;

    return ibuf;
}

/*
 * Note: unlike write_aclip() this will not append a nul.
 */
void write_clip(void *frontend, wchar_t * data, int *attr, int len, int must_deselect)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    HGLOBAL clipdata, clipdata2, clipdata3;
    int len2;
    void *lock, *lock2, *lock3;

    len2 = WideCharToMultiByte(CP_ACP, 0, data, len, 0, 0, NULL, NULL);

    clipdata = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,
			   len * sizeof(wchar_t));
    clipdata2 = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, len2);

    if (!clipdata || !clipdata2) {
	if (clipdata)
	    GlobalFree(clipdata);
	if (clipdata2)
	    GlobalFree(clipdata2);
	return;
    }
    if (!(lock = GlobalLock(clipdata)))
	return;
    if (!(lock2 = GlobalLock(clipdata2)))
	return;

    memcpy(lock, data, len * sizeof(wchar_t));
    WideCharToMultiByte(CP_ACP, 0, data, len, (CHAR*)lock2, len2, NULL, NULL);

    if (conf_get_int( puttyController->cfg, CONF_rtf_paste)) {
	wchar_t unitab[256];
	char *rtf = NULL;
	unsigned char *tdata = (unsigned char *)lock2;
	wchar_t *udata = (wchar_t *)lock;
	int rtflen = 0, uindex = 0, tindex = 0;
	int rtfsize = 0;
	int multilen, blen, alen, totallen, i;
	char before[16], after[4];
	int fgcolour,  lastfgcolour  = 0;
	int bgcolour,  lastbgcolour  = 0;
	int attrBold,  lastAttrBold  = 0;
	int attrUnder, lastAttrUnder = 0;
	int palette[NALLCOLOURS];
	int numcolours;

	get_unitab(CP_ACP, unitab, 0);

	FontSpec* font = conf_get_fontspec(puttyController->cfg, CONF_font);
	rtfsize = 100 + strlen(font->name);
	rtf = snewn(rtfsize, char);
	rtflen = sprintf(rtf, "{\\rtf1\\ansi\\deff0{\\fonttbl\\f0\\fmodern %s;}\\f0\\fs%d",
		font->name, font->height*2);

	/*
	 * Add colour palette
	 * {\colortbl ;\red255\green0\blue0;\red0\green0\blue128;}
	 */

	/*
	 * First - Determine all colours in use
	 *    o  Foregound and background colours share the same palette
	 */
	if (attr) {
	    memset(palette, 0, sizeof(palette));
	    for (i = 0; i < (len-1); i++) {
		fgcolour = ((attr[i] & ATTR_FGMASK) >> ATTR_FGSHIFT);
		bgcolour = ((attr[i] & ATTR_BGMASK) >> ATTR_BGSHIFT);

		if (attr[i] & ATTR_REVERSE) {
		    int tmpcolour = fgcolour;	/* Swap foreground and background */
		    fgcolour = bgcolour;
		    bgcolour = tmpcolour;
		}

		if (puttyController->bold_font_mode == BOLD_NONE && (attr[i] & ATTR_BOLD)) {
		    if (fgcolour  <   8)	/* ANSI colours */
			fgcolour +=   8;
		    else if (fgcolour >= 256)	/* Default colours */
			fgcolour ++;
		}

		if (attr[i] & ATTR_BLINK) {
		    if (bgcolour  <   8)	/* ANSI colours */
			bgcolour +=   8;
    		    else if (bgcolour >= 256)	/* Default colours */
			bgcolour ++;
		}

		palette[fgcolour]++;
		palette[bgcolour]++;
	    }

	    /*
	     * Next - Create a reduced palette
	     */
	    numcolours = 0;
	    for (i = 0; i < NALLCOLOURS; i++) {
		if (palette[i] != 0)
		    palette[i]  = ++numcolours;
	    }

	    /*
	     * Finally - Write the colour table
	     */
	    rtf = sresize(rtf, rtfsize + (numcolours * 25), char);
	    strcat(rtf, "{\\colortbl ;");
	    rtflen = strlen(rtf);

	    for (i = 0; i < NALLCOLOURS; i++) {
		if (palette[i] != 0) {
		    rtflen += sprintf(&rtf[rtflen], "\\red%d\\green%d\\blue%d;", puttyController->defpal[i].rgbtRed, puttyController->defpal[i].rgbtGreen, puttyController->defpal[i].rgbtBlue);
		}
	    }
	    strcpy(&rtf[rtflen], "}");
	    rtflen ++;
	}

	/*
	 * We want to construct a piece of RTF that specifies the
	 * same Unicode text. To do this we will read back in
	 * parallel from the Unicode data in `udata' and the
	 * non-Unicode data in `tdata'. For each character in
	 * `tdata' which becomes the right thing in `udata' when
	 * looked up in `unitab', we just copy straight over from
	 * tdata. For each one that doesn't, we must WCToMB it
	 * individually and produce a \u escape sequence.
	 * 
	 * It would probably be more robust to just bite the bullet
	 * and WCToMB each individual Unicode character one by one,
	 * then MBToWC each one back to see if it was an accurate
	 * translation; but that strikes me as a horrifying number
	 * of Windows API calls so I want to see if this faster way
	 * will work. If it screws up badly we can always revert to
	 * the simple and slow way.
	 */
	while (tindex < len2 && uindex < len &&
	       tdata[tindex] && udata[uindex]) {
	    if (tindex + 1 < len2 &&
		tdata[tindex] == '\r' &&
		tdata[tindex+1] == '\n') {
		tindex++;
		uindex++;
            }

            /*
             * Set text attributes
             */
            if (attr) {
                if (rtfsize < rtflen + 64) {
		    rtfsize = rtflen + 512;
		    rtf = sresize(rtf, rtfsize, char);
                }

                /*
                 * Determine foreground and background colours
                 */
                fgcolour = ((attr[tindex] & ATTR_FGMASK) >> ATTR_FGSHIFT);
                bgcolour = ((attr[tindex] & ATTR_BGMASK) >> ATTR_BGSHIFT);

		if (attr[tindex] & ATTR_REVERSE) {
		    int tmpcolour = fgcolour;	    /* Swap foreground and background */
		    fgcolour = bgcolour;
		    bgcolour = tmpcolour;
		}

		if (puttyController->bold_font_mode == BOLD_NONE && (attr[tindex] & ATTR_BOLD)) {
		    if (fgcolour  <   8)	    /* ANSI colours */
			fgcolour +=   8;
		    else if (fgcolour >= 256)	    /* Default colours */
			fgcolour ++;
                }

		if (attr[tindex] & ATTR_BLINK) {
		    if (bgcolour  <   8)	    /* ANSI colours */
			bgcolour +=   8;
		    else if (bgcolour >= 256)	    /* Default colours */
			bgcolour ++;
                }

                /*
                 * Collect other attributes
                 */
		if (puttyController->bold_font_mode != BOLD_NONE)
		    attrBold  = attr[tindex] & ATTR_BOLD;
		else
		    attrBold  = 0;
                
		attrUnder = attr[tindex] & ATTR_UNDER;

                /*
                 * Reverse video
		 *   o  If video isn't reversed, ignore colour attributes for default foregound
	         *	or background.
		 *   o  Special case where bolded text is displayed using the default foregound
		 *      and background colours - force to bolded RTF.
                 */
		if (!(attr[tindex] & ATTR_REVERSE)) {
		    if (bgcolour >= 256)	    /* Default color */
			bgcolour  = -1;		    /* No coloring */

		    if (fgcolour >= 256) {	    /* Default colour */
			if (puttyController->bold_font_mode == BOLD_NONE && (fgcolour & 1) && bgcolour == -1)
			    attrBold = ATTR_BOLD;   /* Emphasize text with bold attribute */

			fgcolour  = -1;		    /* No coloring */
		    }
		}

                /*
                 * Write RTF text attributes
                 */
		if (lastfgcolour != fgcolour) {
                    lastfgcolour  = fgcolour;
		    rtflen       += sprintf(&rtf[rtflen], "\\cf%d ", (fgcolour >= 0) ? palette[fgcolour] : 0);
                }

                if (lastbgcolour != bgcolour) {
                    lastbgcolour  = bgcolour;
                    rtflen       += sprintf(&rtf[rtflen], "\\highlight%d ", (bgcolour >= 0) ? palette[bgcolour] : 0);
                }

		if (lastAttrBold != attrBold) {
		    lastAttrBold  = attrBold;
		    rtflen       += sprintf(&rtf[rtflen], "%s", attrBold ? "\\b " : "\\b0 ");
		}

                if (lastAttrUnder != attrUnder) {
                    lastAttrUnder  = attrUnder;
                    rtflen        += sprintf(&rtf[rtflen], "%s", attrUnder ? "\\ul " : "\\ulnone ");
                }
	    }

	    if (unitab[tdata[tindex]] == udata[uindex]) {
		multilen = 1;
		before[0] = '\0';
		after[0] = '\0';
		blen = alen = 0;
	    } else {
		multilen = WideCharToMultiByte(CP_ACP, 0, unitab+uindex, 1,
					       NULL, 0, NULL, NULL);
		if (multilen != 1) {
		    blen = sprintf(before, "{\\uc%d\\u%d", multilen,
				   udata[uindex]);
		    alen = 1; strcpy(after, "}");
		} else {
		    blen = sprintf(before, "\\u%d", udata[uindex]);
		    alen = 0; after[0] = '\0';
		}
	    }
	    assert(tindex + multilen <= len2);
	    totallen = blen + alen;
	    for (i = 0; i < multilen; i++) {
		if (tdata[tindex+i] == '\\' ||
		    tdata[tindex+i] == '{' ||
		    tdata[tindex+i] == '}')
		    totallen += 2;
		else if (tdata[tindex+i] == 0x0D || tdata[tindex+i] == 0x0A)
		    totallen += 6;     /* \par\r\n */
		else if (tdata[tindex+i] > 0x7E || tdata[tindex+i] < 0x20)
		    totallen += 4;
		else
		    totallen++;
	    }

	    if (rtfsize < rtflen + totallen + 3) {
		rtfsize = rtflen + totallen + 512;
		rtf = sresize(rtf, rtfsize, char);
	    }

	    strcpy(rtf + rtflen, before); rtflen += blen;
	    for (i = 0; i < multilen; i++) {
		if (tdata[tindex+i] == '\\' ||
		    tdata[tindex+i] == '{' ||
		    tdata[tindex+i] == '}') {
		    rtf[rtflen++] = '\\';
		    rtf[rtflen++] = tdata[tindex+i];
		} else if (tdata[tindex+i] == 0x0D || tdata[tindex+i] == 0x0A) {
		    rtflen += sprintf(rtf+rtflen, "\\par\r\n");
		} else if (tdata[tindex+i] > 0x7E || tdata[tindex+i] < 0x20) {
		    rtflen += sprintf(rtf+rtflen, "\\'%02x", tdata[tindex+i]);
		} else {
		    rtf[rtflen++] = tdata[tindex+i];
		}
	    }
	    strcpy(rtf + rtflen, after); rtflen += alen;

	    tindex += multilen;
	    uindex++;
	}

        rtf[rtflen++] = '}';	       /* Terminate RTF stream */
        rtf[rtflen++] = '\0';
        rtf[rtflen++] = '\0';

	clipdata3 = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, rtflen);
	if (clipdata3 && (lock3 = GlobalLock(clipdata3)) != NULL) {
	    memcpy(lock3, rtf, rtflen);
	    GlobalUnlock(clipdata3);
	}
	sfree(rtf);
    } else
	clipdata3 = NULL;

    GlobalUnlock(clipdata);
    GlobalUnlock(clipdata2);

    if (!must_deselect)
	SendMessage(puttyController->getNativePage(), WM_IGNORE_CLIP, TRUE, 0);

    if (OpenClipboard(puttyController->getNativePage())) {
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, clipdata);
	SetClipboardData(CF_TEXT, clipdata2);
	if (clipdata3)
		SetClipboardData(RegisterClipboardFormat(L"Rich Text Format"), clipdata3);
	CloseClipboard();
    } else {
	GlobalFree(clipdata);
	GlobalFree(clipdata2);
    }

    if (!must_deselect)
	SendMessage(puttyController->getNativePage(), WM_IGNORE_CLIP, FALSE, 0);
}

void get_clip(void *frontend, wchar_t **p, int *len)
{
    if (p) {
	*p = clipboard_contents;
	*len = clipboard_length;
    }
}

static DWORD WINAPI clipboard_read_threadfunc(void *param)
{
    HWND hwnd = (HWND)param;
    HGLOBAL clipdata;

    if (OpenClipboard(NULL)) {
	if ((clipdata = GetClipboardData(CF_UNICODETEXT))) {
	    SendMessage(hwnd, WM_GOT_CLIPDATA, (WPARAM)1, (LPARAM)clipdata);
	} else if ((clipdata = GetClipboardData(CF_TEXT))) {
	    SendMessage(hwnd, WM_GOT_CLIPDATA, (WPARAM)0, (LPARAM)clipdata);
	}
	CloseClipboard();
    }

    return 0;
}

void request_paste(void *frontend)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    /*
     * I always thought pasting was synchronous in Windows; the
     * clipboard access functions certainly _look_ synchronous,
     * unlike the X ones. But in fact it seems that in some
     * situations the contents of the clipboard might not be
     * immediately available, and the clipboard-reading functions
     * may block. This leads to trouble if the application
     * delivering the clipboard data has to get hold of it by -
     * for example - talking over a network connection which is
     * forwarded through this very PuTTY.
     *
     * Hence, we spawn a subthread to read the clipboard, and do
     * our paste when it's finished. The thread will send a
     * message back to our main window when it terminates, and
     * that tells us it's OK to paste.
     */
    DWORD in_threadid; /* required for Win9x */
    CreateThread(NULL, 0, clipboard_read_threadfunc,
		 puttyController->getNativePage(), 0, &in_threadid);
}

int process_clipdata(HGLOBAL clipdata, int unicode)
{
    sfree(clipboard_contents);
    clipboard_contents = NULL;
    clipboard_length = 0;

    if (unicode) {
	wchar_t *p = (wchar_t*)GlobalLock(clipdata);
	wchar_t *p2;

	if (p) {
	    /* Unwilling to rely on Windows having wcslen() */
	    for (p2 = p; *p2; p2++);
	    clipboard_length = p2 - p;
	    clipboard_contents = snewn(clipboard_length + 1, wchar_t);
	    memcpy(clipboard_contents, p, clipboard_length * sizeof(wchar_t));
	    clipboard_contents[clipboard_length] = L'\0';
	    return TRUE;
	}
    } else {
	char *s = (char*)GlobalLock(clipdata);
	int i;

	if (s) {
	    i = MultiByteToWideChar(CP_ACP, 0, s, strlen(s) + 1, 0, 0);
	    clipboard_contents = snewn(i, wchar_t);
	    MultiByteToWideChar(CP_ACP, 0, s, strlen(s) + 1,
				clipboard_contents, i);
	    clipboard_length = i - 1;
	    clipboard_contents[clipboard_length] = L'\0';
	    return TRUE;
	}
    }

    return FALSE;
}

void palette_set(void *frontend, int n, int r, int g, int b)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    
    if (n >= 16)
	n += 256 - 16;
    if (n > NALLCOLOURS)
	return;
    puttyController->real_palette_set(n, r, g, b);
    if (puttyController->pal) {
        get_ctx(frontend);
    	HDC hdc = puttyController->hdc;
        assert (hdc != NULL);
    	UnrealizeObject(puttyController->pal);
    	RealizePalette(hdc);
    	free_ctx(frontend, (Context)frontend);
    } else {
	if (n == (ATTR_DEFBG>>ATTR_BGSHIFT))
	    /* If Default Background changes, we need to ensure any
	     * space between the text area and the window border is
	     * redrawn. */
	    InvalidateRect(puttyController->getNativePage(), NULL, TRUE);
    }
}

void palette_reset(void *frontend)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    int i;

    /* And this */
    for (i = 0; i < NALLCOLOURS; i++) {
	if (puttyController->pal) {
	    puttyController->logpal->palPalEntry[i].peRed = puttyController->defpal[i].rgbtRed;
	    puttyController->logpal->palPalEntry[i].peGreen = puttyController->defpal[i].rgbtGreen;
	    puttyController->logpal->palPalEntry[i].peBlue = puttyController->defpal[i].rgbtBlue;
	    puttyController->logpal->palPalEntry[i].peFlags = 0;
	    puttyController->colours[i] = PALETTERGB(puttyController->defpal[i].rgbtRed,
				    puttyController->defpal[i].rgbtGreen,
				    puttyController->defpal[i].rgbtBlue);
	} else
	    puttyController->colours[i] = RGB(puttyController->defpal[i].rgbtRed,
			     puttyController->defpal[i].rgbtGreen, puttyController->defpal[i].rgbtBlue);
    }

    if (puttyController->pal) {
	HDC hdc;
	SetPaletteEntries(puttyController->pal, 0, NALLCOLOURS, puttyController->logpal->palPalEntry);
	get_ctx(frontend);
    hdc = puttyController->hdc;
    assert (hdc != NULL);
	RealizePalette(hdc);
	free_ctx(frontend, (Context)frontend);
    } else {
    	/* Default Background may have changed. Ensure any space between
    	 * text area and window border is redrawn. */
    	InvalidateRect(puttyController->getNativePage(), NULL, TRUE);
    }
}

/*
 * Beep.
 */
void do_beep(void *frontend, int mode)
{
	USES_CONVERSION;
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    if (mode == BELL_DEFAULT) {
	/*
	 * For MessageBeep style bells, we want to be careful of
	 * timing, because they don't have the nice property of
	 * PlaySound bells that each one cancels the previous
	 * active one. So we limit the rate to one per 50ms or so.
	 */
	static long lastbeep = 0;
	long beepdiff;

	beepdiff = GetTickCount() - lastbeep;
	if (beepdiff >= 0 && beepdiff < 50)
	    return;
	MessageBeep(MB_OK);
	/*
	 * The above MessageBeep call takes time, so we record the
	 * time _after_ it finishes rather than before it starts.
	 */
	lastbeep = GetTickCount();
    } else if (mode == BELL_WAVEFILE) {
		Filename* file = conf_get_filename(puttyController->cfg, CONF_bell_wavefile);
		if (!PlaySound(A2W(file->path), NULL,
		       SND_ASYNC | SND_FILENAME)) {
	    char buf[1024 + 80];
	    char otherbuf[100];
	    sprintf(buf, "Unable to play sound file\n%s\n"
		    "Using default sound instead", file->path);
	    sprintf(otherbuf, "%.70s Sound Error", appname);
	    MessageBox(WindowInterface::GetInstance()->getNativeTopWnd(), A2W(buf), A2W(otherbuf),
			MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST);
	    conf_set_int( puttyController->cfg, CONF_beep, BELL_DEFAULT);
	}
    } else if (mode == BELL_PCSPEAKER) {
	static long lastbeep = 0;
	long beepdiff;

	beepdiff = GetTickCount() - lastbeep;
	if (beepdiff >= 0 && beepdiff < 50)
	    return;

	/*
	 * We must beep in different ways depending on whether this
	 * is a 95-series or NT-series OS.
	 */
	OSVERSIONINFO& osVersion = get_os_version();
	if(osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT)
	    Beep(800, 100);
	else
	    MessageBeep(-1);
	lastbeep = GetTickCount();
    }
    /* Otherwise, either visual bell or disabled; do nothing here */
    if (!puttyController->term->has_focus) {
		puttyController->flash_window(2);	       /* start */

    }
}

/*
 * Minimise or restore the window in response to a server-side
 * request.
 */
void set_iconic(void *frontend, int iconic)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    if (IsIconic(puttyController->getNativePage())) {
	if (!iconic)
	    ShowWindow(puttyController->getNativePage(), SW_RESTORE);
    } else {
	if (iconic)
	    ShowWindow(puttyController->getNativePage(), SW_MINIMIZE);
    }
}

/*
 * Move the window in response to a server-side request.
 */
void move_window(void *frontend, int x, int y)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
	int resize_action = conf_get_int( puttyController->cfg, CONF_resize_action);
    if (resize_action == RESIZE_DISABLED || 
        resize_action == RESIZE_FONT ||
		IsZoomed(WindowInterface::GetInstance()->getNativeTopWnd()))
       return;

    SetWindowPos(puttyController->getNativePage(), NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/*
 * Move the window to the top or bottom of the z-order in response
 * to a server-side request.
 */
void set_zorder(void *frontend, int top)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    if (conf_get_int( puttyController->cfg, CONF_alwaysontop))
	return;			       /* ignore */
    SetWindowPos(puttyController->getNativePage(), top ? HWND_TOP : HWND_BOTTOM, 0, 0, 0, 0,
		 SWP_NOMOVE | SWP_NOSIZE);
}

/*
 * Refresh the window in response to a server-side request.
 */
void refresh_window(void *frontend)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    InvalidateRect(puttyController->getNativePage(), NULL, TRUE);
}

/*
 * Maximise or restore the window in response to a server-side
 * request.
 */
void set_zoomed(void *frontend, int zoomed)
{
    if (IsZoomed(WindowInterface::GetInstance()->getNativeTopWnd())) {
        if (!zoomed)
	    ShowWindow(WindowInterface::GetInstance()->getNativeTopWnd(), SW_RESTORE);
    } else {
	if (zoomed)
	    ShowWindow(WindowInterface::GetInstance()->getNativeTopWnd(), SW_MAXIMIZE);
    }
}

/*
 * Report whether the window is iconic, for terminal reports.
 */
int is_iconic(void *frontend)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    return IsIconic(puttyController->getNativePage());
}

/*
 * Report the window's position, for terminal reports.
 */
void get_window_pos(void *frontend, int *x, int *y)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    RECT r;
    GetWindowRect(puttyController->getNativePage(), &r);
    *x = r.left;
    *y = r.top;
}

/*
 * Report the window's pixel size, for terminal reports.
 */
void get_window_pixels(void *frontend, int *x, int *y)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    RECT r;
    GetWindowRect(puttyController->getNativePage(), &r);
    *x = r.right - r.left;
    *y = r.bottom - r.top;
}

/*
 * Return the window or icon title.
 */
char *get_window_title(void *frontend, int icon)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    return icon ? puttyController->icon_name : puttyController->window_name;
}


void request_resize(void *frontend, int w, int h)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
 //   
 //   int width, height;
 //   int extra_width, extra_height;
 //   wintabitem_get_extra_size(tabitem, &extra_width, &extra_height);

 //   /* If the window is maximized supress resizing attempts */
 //   if (IsZoomed(hwnd)) {
 //   	if (puttyController->cfg->resize_action == RESIZE_TERM)
 //   	    return;
 //   }

 //   if (puttyController->cfg->resize_action == RESIZE_DISABLED) return;
 //   if (h == puttyController->term->rows && w == puttyController->term->cols) return;

 //   /* Sanity checks ... */
 //   {
 //   	static int first_time = 1;
 //   	static RECT ss;

 //   	switch (first_time) {
 //   	  case 1:
 //   	    /* Get the size of the screen */
 //   	    if (get_fullscreen_rect(&ss))
 //   		/* first_time = 0 */ ;
 //   	    else {
 //   		first_time = 2;
 //   		break;
 //   	    }
 //   	  case 0:
 //   	    /* Make sure the values are sane */
 //   	    width = (ss.right - ss.left - extra_width) / 4;
 //   	    height = (ss.bottom - ss.top - extra_height) / 6;

 //   	    if (w > width || h > height)
 //   		return;
 //   	    if (w < 15)
 //   		w = 15;
 //   	    if (h < 1)
 //   		h = 1;
 //   	}
 //   }

 //   term_size(puttyController->term, h, w, puttyController->cfg->savelines);

 //   if (puttyController->cfg->resize_action != RESIZE_FONT && !IsZoomed(hwnd)) {
	//width = puttyController->font_width * w;
	//height = puttyController->font_height * h;

	//wintabitem_require_resize(tabitem, width, height);
 //   } else
	//reset_window(tabitem, 0);

 //   InvalidateRect(hwnd, NULL, TRUE);
}


void set_title(void *frontend, char *title)
{
    assert (frontend != NULL);
    if (!title || !*title) return;
    int titlelen = strlen(title);
    if (titlelen <= 0 || titlelen >= 64) return;
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    sfree(puttyController->window_name);
    puttyController->window_name = snewn(1 + strlen(title), char);
    strcpy(puttyController->window_name, title);
    if (conf_get_int( puttyController->cfg, CONF_win_name_always) || !IsIconic(WindowInterface::GetInstance()->getNativeTopWnd()))
//	SetWindowText(hwnd, title);
	if (!conf_get_int( puttyController->cfg, CONF_no_remote_tabname)){
		if (in_utf(puttyController->term))
		{
			USES_CONVERSION;
			string16 w_title = A2W_CP(title, CP_UTF8);
			char* sys_title = W2A(w_title.c_str());
			puttyController->rename(sys_title);
		}
		else
		{
			puttyController->rename(title);
		}
	}
	
}

void set_icon(void *frontend, char *title)
{
    assert (frontend != NULL);
    if (!title || !*title) return;
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    sfree(puttyController->icon_name);
    puttyController->icon_name = snewn(1 + strlen(title), char);
    strcpy(puttyController->icon_name, title);
    //if (!puttyController->cfg->win_name_always && IsIconic(hwnd))
	//SetWindowText(hwnd, title);
}

//----------------------------------------------------------------------------------
//for logging

//-----------------------------------------------------------------------------------
//for misc
/*
 * Print a modal (Really Bad) message box and perform a fatal exit.
 */
void modalfatalbox(char *fmt, ...)
{
	USES_CONVERSION;
    va_list ap;
    char *stuff, morestuff[100];

    va_start(ap, fmt);
    stuff = dupvprintf(fmt, ap);
    va_end(ap);
    sprintf(morestuff, "%.70s Fatal Error", appname);
    MessageBox(WindowInterface::GetInstance()->getNativeTopWnd(), A2W(stuff), A2W(morestuff),
		MB_SYSTEMMODAL | MB_ICONERROR | MB_OK | MB_TOPMOST);
    sfree(stuff);
    cleanup_exit(1);
}

void frontend_keypress(void *handle)
{
    /*
     * Keypress termination in non-Close-On-Exit mode is not
     * currently supported in PuTTY proper, because the window
     * always has a perfectly good Close button anyway. So we do
     * nothing here.
     */
    return;
}

/* Dummy routine, only required in plink. */
void ldisc_update(void *frontend, int echo, int edit)
{
}
//---------------------------------------------------------------------------------
// for backend
int from_backend(void *frontend, int is_stderr, const char *data, int len)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
	if (1){
		if (puttyController->checkZSession(data, len)){
			return 500;
		}
		return term_data(puttyController->term, is_stderr, data, len);

	}else{
		return term_data(puttyController->term, is_stderr, data, len);
	}
}

int from_backend_untrusted(void *frontend, const char *data, int len)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    return term_data_untrusted(puttyController->term, data, len);
}

int from_backend_eof(void *frontend)
{
    return TRUE;   /* do respond to incoming EOF with outgoing */
}

void frontend_echoedit_update(void *frontend, int echo, int edit)
{
}
//---------------------------------------------------------------
//for settings

//-----------------------------------------------------------------
//for timing
#include "putty_timer.h"
void timer_change_notify(unsigned long next)
{
	PuttyTimer::GetInstance()->start(next);
}

//-----------------------------------------------------------------
//for windlg

RECT getMaxWorkArea()
{
//	RECT WorkArea;
	HMONITOR mon;
	MONITORINFO mi;
	mon = MonitorFromWindow(WindowInterface::GetInstance()->getNativeTopWnd(), MONITOR_DEFAULTTONEAREST);
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(mon, &mi);
	
	/* structure copy */
	return /*fullscr_on_max ? mi.rcMonitor:*/ mi.rcWork;
}

void write_aclip(void *frontend, char *data, int len, int must_deselect)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    HGLOBAL clipdata;
    void *lock;

    clipdata = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, len + 1);
    if (!clipdata)
	return;
    lock = GlobalLock(clipdata);
    if (!lock)
	return;
    memcpy(lock, data, len);
    ((unsigned char *) lock)[len] = 0;
    GlobalUnlock(clipdata);

    if (!must_deselect)
	SendMessage(puttyController->getNativePage(), WM_IGNORE_CLIP, TRUE, 0);

    if (OpenClipboard(puttyController->getNativePage())) {
	EmptyClipboard();
	SetClipboardData(CF_TEXT, clipdata);
	CloseClipboard();
    } else
	GlobalFree(clipdata);

    if (!must_deselect)
	SendMessage(puttyController->getNativePage(), WM_IGNORE_CLIP, FALSE, 0);
}
//----------------------------------------------------------------------------
//backend
/*
 * Update the Special Commands submenu.
 */
void update_specials_menu(void *frontend)
{
	USES_CONVERSION;
    assert (frontend != NULL);

    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    HMENU new_menu;
    int i;//, j;

    if (puttyController->back)
	puttyController->specials = puttyController->back->get_specials(puttyController->backhandle);
    else
	puttyController->specials = NULL;

    if (puttyController->specials) {
	/* We can't use Windows to provide a stack for submenus, so
	 * here's a lame "stack" that will do for now. */
	HMENU saved_menu = NULL;
	int nesting = 1;
	new_menu = CreatePopupMenu();
	for (i = 0; nesting > 0; i++) {
	    assert(IDM_SPECIAL_MIN + 0x10 * i < IDM_SPECIAL_MAX);
	    switch (puttyController->specials[i].code) {
	      case TS_SEP:
		AppendMenu(new_menu, MF_SEPARATOR, 0, 0);
		break;
	      case TS_SUBMENU:
		assert(nesting < 2);
		nesting++;
		saved_menu = new_menu; /* XXX lame stacking */
		new_menu = CreatePopupMenu();
		AppendMenu(saved_menu, MF_POPUP | MF_ENABLED,
			   (UINT) new_menu, A2W(puttyController->specials[i].name));
		break;
	      case TS_EXITMENU:
		nesting--;
		if (nesting) {
		    new_menu = saved_menu; /* XXX lame stacking */
		    saved_menu = NULL;
		}
		break;
	      default:
		AppendMenu(new_menu, MF_ENABLED, IDM_SPECIAL_MIN + 0x10 * i,
			   A2W(puttyController->specials[i].name));
		break;
	    }
	}
	/* Squirrel the highest special. */
	puttyController->n_specials = i - 1;
    } else {
	new_menu = NULL;
	puttyController->n_specials = 0;
    }
    /* not show the special menu 
    for (j = 0; j < lenof(popup_menus); j++) {
	if (puttyController->specials_menu) {
	    // XXX does this free up all submenus? 
	    DeleteMenu(popup_menus[j].menu, (UINT)specials_menu, MF_BYCOMMAND);
	    DeleteMenu(popup_menus[j].menu, IDM_SPECIALSEP, MF_BYCOMMAND);
	}
	if (new_menu) {
	    InsertMenu(popup_menus[j].menu, IDM_SHOWLOG,
		       MF_BYCOMMAND | MF_POPUP | MF_ENABLED,
		       (UINT) new_menu, "S&pecial Command");
	    InsertMenu(popup_menus[j].menu, IDM_SHOWLOG,
		       MF_BYCOMMAND | MF_SEPARATOR, IDM_SPECIALSEP, 0);
	}
    }*/
    puttyController->specials_menu = new_menu;
}

void notify_remote_exit(void *frontend)
{
	USES_CONVERSION;
    int exitcode;
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
	puttyController->setDisconnected();

    if (!puttyController->session_closed &&
        (exitcode = puttyController->back->exitcode(puttyController->backhandle)) >= 0) {
	/* Abnormal exits will already have set session_closed and taken
	 * appropriate action. */
	int close_on_exit = conf_get_int(puttyController->cfg, CONF_close_on_exit);
	if (close_on_exit == FORCE_ON ||
	    (close_on_exit == AUTO && exitcode != INT_MAX)) {
	    //wintabitem_close_session(tabitem);
	    
	    puttyController->must_close_session = TRUE;
	    puttyController->session_closed = TRUE;
		puttyController->must_close_tab_ = TRUE;

	} else {
	    puttyController->must_close_session = TRUE;
	    puttyController->session_closed = TRUE;
	    /* exitcode == INT_MAX indicates that the connection was closed
	     * by a fatal error, so an error box will be coming our way and
	     * we should not generate this informational one. */
	    if (exitcode != INT_MAX){
			logevent(puttyController, "Connection closed by remote host");
			const char* str = "\r\n"
			"===============================================================\r\n"
			"--------         Connection closed by remote host      --------\r\n"
			"===============================================================\r\n";
			term_data(puttyController->term, 1, str, strlen(str));
		//MessageBox(WindowInterface::GetInstance()->getNativeTopWnd(), L"Connection closed by remote host",
		//	   A2W(appname), MB_OK | MB_ICONINFORMATION);
	    }
	}
    }
}

/*
 * Print a message box and close the connection.
 */
void connection_fatal(void *frontend, const char * const fmt, ...)
{
	USES_CONVERSION;
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    
    va_list ap;
    char *stuff, morestuff[100];
	char msg[2048] = { 0 };

    va_start(ap, fmt);
    stuff = dupvprintf(fmt, ap);
    va_end(ap);

	snprintf(msg, sizeof(msg) - 1 ,"\r\n"
		"===============================================================\r\n"
		"------                    Fatal Error                          \r\n"
		"------Error:%s\r\n"
		"===============================================================\r\n"
		, stuff);
	term_data(puttyController->term, 1, msg, strlen(msg));

    sfree(stuff);
    
}

void set_busy_status(void *frontend, int status)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    puttyController->busy_status = status;
    puttyController->update_mouse_pointer();
}

bool is_autocmd_enabled(void *frontend)
{
	assert(frontend != NULL);
	NativePuttyController *puttyController = (NativePuttyController *)frontend;
	return puttyController->isAutoCmdEnabled_;
}

void set_autocmd_enabled(void *frontend, bool enabled)
{
	assert(frontend != NULL);
	NativePuttyController *puttyController = (NativePuttyController *)frontend;
	puttyController->isAutoCmdEnabled_ = enabled;
}

int get_userpass_input(void *frontend, prompts_t *p, const unsigned char * in, int inlen)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    int ret;
    ret = autocmd_get_passwd_input(frontend, p, puttyController->term->conf);
	set_autocmd_enabled(frontend, FALSE);
    if (ret == -1)
        ret = cmdline_get_passwd_input(p, in, inlen);
    if (ret == -1)
		ret = term_get_userpass_input(puttyController->term, p, in, inlen);
	set_autocmd_enabled(frontend, TRUE);
    return ret;
}

char *get_ttymode(void *frontend, const char *mode)
{
    assert(frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    return term_get_ttymode(puttyController->term, mode);
}

//-----------------------------------------------------------------------------
//for winnet.c
/*
 * Set up, or shut down, an AsyncSelect. Called from winnet.c.
 */
char *do_select(void* frontend, SOCKET skt, int startup)
{
	assert(frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    int msg, events;
	HWND hwnd = NULL;
    if (startup) {
		if (!puttyController->getNativePage())
			return "do_select(): internal error (hwnd==NULL)";
		hwnd = puttyController->getNativePage();
		msg = WM_NETEVENT;
		events = (FD_CONNECT | FD_READ | FD_WRITE |
			  FD_OOB | FD_CLOSE | FD_ACCEPT);
	} else {
		msg = events = 0;
	}
	if (p_WSAAsyncSelect(skt, hwnd, msg, events) == SOCKET_ERROR) {
		switch (p_WSAGetLastError()) {
			case WSAENETDOWN:
			return "Network is down";
			default:
			return "WSAAsyncSelect(): unknown error";
		}
    }
    return NULL;
}

/*
 * Clean up and exit.
 */
void cleanup_exit(int code)
{
    /*
     * Clean up.
     */
    //wintab_fini(&tab);
    sk_cleanup();

    shutdown_help();

    /* Clean up COM. */
    CoUninitialize();
//    CloseHandle(config_dialog_mutex);

    exit(code);
}


/*
 * Report an error at the command-line parsing stage.
 */
void cmdline_error(const char *fmt, ...)
{
	USES_CONVERSION;
    va_list ap;
    char *stuff, morestuff[100];

    va_start(ap, fmt);
    stuff = dupvprintf(fmt, ap);
    va_end(ap);
    sprintf(morestuff, "%.70s Command Line Error", appname);
	MessageBox(WindowInterface::GetInstance()->getNativeTopWnd(), A2W(stuff), A2W(morestuff), MB_ICONERROR | MB_OK | MB_TOPMOST);
    sfree(stuff);
    exit(1);
}

#include "win_res.h"
void logevent(void *frontend, const char *str)
{
    if (frontend == NULL){
        debug(("%s\n", str));
        return;
    }
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    char timebuf[40];
    struct tm tm;
    int i;

    log_eventlog(puttyController->logctx, str);

    if (puttyController->nevents >= puttyController->negsize) {
    	puttyController->negsize += 64;
    	puttyController->events = sresize(puttyController->events, puttyController->negsize, char *);
        for (i = puttyController->nevents + 1; i < puttyController->negsize; i++)
            puttyController->events[i] = NULL;
    }

    tm=ltime();
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S\t", &tm);

    puttyController->events[puttyController->nevents] = snewn(strlen(timebuf) + strlen(str) + 1, char);
    strcpy(puttyController->events[puttyController->nevents], timebuf);
    strcat(puttyController->events[puttyController->nevents], str);
    if (puttyController->logbox) {
	int count;
	SendDlgItemMessage(puttyController->logbox, IDN_LIST, LB_ADDSTRING,
			   0, (LPARAM) puttyController->events[puttyController->nevents]);
	count = SendDlgItemMessage(puttyController->logbox, IDN_LIST, LB_GETCOUNT, 0, 0);
	SendDlgItemMessage(puttyController->logbox, IDN_LIST, LB_SETTOPINDEX, count - 1, 0);
    }
    puttyController->nevents++;
}

static int CALLBACK LogProc(HWND hwnd, UINT msg,
			    WPARAM wParam, LPARAM lParam)
{
	USES_CONVERSION;
    int i;
	if (msg == WM_INITDIALOG){
		win_bind_data(hwnd, (void*)lParam);
	}
	NativePuttyController *puttyController = (NativePuttyController *)win_get_data(hwnd);
    if (puttyController == NULL){
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
      case WM_INITDIALOG:
	{
	    char *str = dupprintf("%s Event Log", appname);
	    SetWindowText(hwnd, A2W(str));
	    sfree(str);
	}
	{
	    static int tabs[4] = { 78, 108 };
	    SendDlgItemMessage(hwnd, IDN_LIST, LB_SETTABSTOPS, 2,
			       (LPARAM) tabs);
	}
	for (i = 0; i < puttyController->nevents; i++)
	    SendDlgItemMessage(hwnd, IDN_LIST, LB_ADDSTRING,
			       0, (LPARAM) A2W( puttyController->events[i]));
	return 1;
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
	  case IDOK:
	  case IDCANCEL:
	    puttyController->logbox = NULL;
	    SetActiveWindow(GetParent(hwnd));
	    DestroyWindow(hwnd);
	    return 0;
	  case IDN_COPY:
	    if (HIWORD(wParam) == BN_CLICKED ||
		HIWORD(wParam) == BN_DOUBLECLICKED) {
		int selcount;
		int *selitems;
		selcount = SendDlgItemMessage(hwnd, IDN_LIST,
					      LB_GETSELCOUNT, 0, 0);
		if (selcount == 0) {   /* don't even try to copy zero items */
		    MessageBeep(0);
		    break;
		}

		selitems = snewn(selcount, int);
		if (selitems) {
		    int count = SendDlgItemMessage(hwnd, IDN_LIST,
						   LB_GETSELITEMS,
						   selcount,
						   (LPARAM) selitems);
		    int i;
		    int size;
		    char *clipdata;
		    static unsigned char sel_nl[] = SEL_NL;

		    if (count == 0) {  /* can't copy zero stuff */
			MessageBeep(0);
			break;
		    }

		    size = 0;
		    for (i = 0; i < count; i++)
			size +=
			    strlen(puttyController->events[selitems[i]]) + sizeof(sel_nl);

		    clipdata = snewn(size, char);
		    if (clipdata) {
			char *p = clipdata;
			for (i = 0; i < count; i++) {
			    char *q = puttyController->events[selitems[i]];
			    int qlen = strlen(q);
			    memcpy(p, q, qlen);
			    p += qlen;
			    memcpy(p, sel_nl, sizeof(sel_nl));
			    p += sizeof(sel_nl);
			}
			write_aclip(puttyController, clipdata, size, TRUE);
			sfree(clipdata);
		    }
		    sfree(selitems);

		    for (i = 0; i < puttyController->nevents; i++)
			SendDlgItemMessage(hwnd, IDN_LIST, LB_SETSEL,
					   FALSE, i);
		}
	    }
	    return 0;
	}
	return 0;
      case WM_CLOSE:{
	puttyController->logbox = NULL;
	SetActiveWindow(GetParent(hwnd));
	DestroyWindow(hwnd);
	return 0;
      }
    }
    return 0;
}

void showeventlog(void* frontend, HWND hwnd)
{
    assert (frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    if (!puttyController->logbox) {
	puttyController->logbox = CreateDialogParam(hinst, MAKEINTRESOURCE(IDD_LOGBOX),
			      hwnd, LogProc, (LPARAM)puttyController);
	ShowWindow(puttyController->logbox, SW_SHOWNORMAL);
    }
    SetActiveWindow(puttyController->logbox);
}

int need_cmd_scatter()
{
	return WindowInterface::GetInstance()->ifNeedCmdScat();
}

void cmd_scatter(const char *buf, int len, int interactive)
{
	WindowInterface::GetInstance()->cmdScat(NativePuttyController::LDISC_SEND, buf, len, interactive);
}

int read_lnk(const char* lnk_name, char* link_path, unsigned link_path_len);
struct IniComparer
{
  bool operator()(const std::string& s1, const std::string& s2) const
  {
  	int len1 = s1.length();
	int len2 = s2.length();
	int min_len = len1 > len2 ? len2 : len1;
    return strncmp(s1.c_str(), s2.c_str(), min_len) < 0;
  }
};
typedef std::map<std::string, std::string, IniComparer> IniMap;
int parse_ini_file(const char* filename, IniMap& attrMap)
{
	std::fstream fin(filename);
	char line[256] = {0};
	IniMap::iterator it;
	int i = 0;
	while(fin.getline(line, sizeof (line))){
		if ((it = attrMap.find(line)) != attrMap.end()){
			it->second = std::string(line + it->first.length());
			i++;
		}
		if (i == attrMap.size()){
			break;
		}
	}
	fin.close();
	return 0;
}

const char* const INIKEY_PROTOCOL = "S:\"Protocol Name\"=";
const char* const INIKEY_PRIKEY = "S:\"Identity Filename\"=";
const char* const INIKEY_HOSTNAME = "S:\"Hostname\"=";
const char* const INIKEY_USERNAME = "S:\"Username\"=";
const char* const INIKEY_SERIALPORT = "S:\"Com Port\"=";
const char* const INIKEY_PORT = "D:\"Port\"=";
const char* const INIKEY_SSH1PORT = "D:\"[SSH1] Port\"=";
const char* const INIKEY_SSH2PORT = "D:\"[SSH2] Port\"=";
const char* const INIKEY_BAUD_RATE = "D:\"Baud Rate\"=";
const char* const INIKEY_DATA_BITS = "D:\"Data Bits\"=";
const char* const INIKEY_SE_XON_FLOW = "D:\"XON Flow\"=";
const char* const INIKEY_SE_RTS_FLOW = "D:\"CTS Flow\"=";
const char* const INIKEY_SE_DTR_FLOW = "D:\"DSR Flow\"=";
const char* const INIKEY_SE_PARITY = "D:\"Parity\"=";
const char* const INIKEY_SE_STOP_BITS = "D:\"Stop Bits\"=";

const char* const INIKEY_USEGKEY = "D:\"Use Global Public Key\"=";
const IniMap::value_type init_value[] =
{
	IniMap::value_type( INIKEY_PROTOCOL, ""),
	IniMap::value_type( INIKEY_PRIKEY, ""),
	IniMap::value_type( INIKEY_HOSTNAME, ""),
	IniMap::value_type( INIKEY_USERNAME, ""),
	IniMap::value_type( INIKEY_SERIALPORT, ""),
	IniMap::value_type( INIKEY_PORT, ""),
	IniMap::value_type( INIKEY_SSH1PORT, ""),
	IniMap::value_type( INIKEY_SSH2PORT, ""),
	IniMap::value_type( INIKEY_BAUD_RATE, ""),
	IniMap::value_type( INIKEY_DATA_BITS, ""),
	IniMap::value_type( INIKEY_SE_PARITY, ""),
	IniMap::value_type( INIKEY_SE_STOP_BITS, ""),
	IniMap::value_type( INIKEY_SE_XON_FLOW, ""),
	IniMap::value_type( INIKEY_SE_RTS_FLOW, ""),
	IniMap::value_type( INIKEY_SE_DTR_FLOW, ""),
	IniMap::value_type( INIKEY_USEGKEY, "")
};
const IniMap SESSION_MAP_TMPL(init_value, init_value + sizeof(init_value)/sizeof(IniMap::value_type));
		
void convertSecureCRTData(IniMap& theGlobalMap, IniMap& theSessionMap, Conf* theCfg)
{
	int protocol = theSessionMap[INIKEY_PROTOCOL] == "SSH2" ? PROT_SSH 
		: theSessionMap[INIKEY_PROTOCOL] == "Serial" ? PROT_SERIAL
		: theSessionMap[INIKEY_PROTOCOL] == "RLogin" ? PROT_RLOGIN  
		: theSessionMap[INIKEY_PROTOCOL] == "Telnet" ? PROT_TELNET  
		: theSessionMap[INIKEY_PROTOCOL] == "SSH1" ? PROT_SSH  
		: PROT_SSH;
	conf_set_int( theCfg, CONF_protocol, protocol);
	if (protocol == PROT_SERIAL){
		conf_set_str(theCfg, CONF_serline, theSessionMap[INIKEY_SERIALPORT].c_str());
		int tmp_value;
		sscanf(theSessionMap[INIKEY_BAUD_RATE].c_str(), "%x", &tmp_value);
		conf_set_int(theCfg, CONF_serspeed, tmp_value);
		sscanf(theSessionMap[INIKEY_DATA_BITS].c_str(), "%x", &tmp_value);
		conf_set_int(theCfg, CONF_serdatabits, tmp_value);
		sscanf(theSessionMap[INIKEY_SE_PARITY].c_str(), "%x", &tmp_value);
		conf_set_int(theCfg, CONF_serparity, tmp_value);
		sscanf(theSessionMap[INIKEY_SE_STOP_BITS].c_str(), "%x", &tmp_value);
		conf_set_int(theCfg, CONF_serstopbits, tmp_value*2);
		int isXon = 0, isRts = 0, isDtr = 0;
		sscanf(theSessionMap[INIKEY_SE_XON_FLOW].c_str(), "%x", &isXon);
		sscanf(theSessionMap[INIKEY_SE_RTS_FLOW].c_str(), "%x", &isRts);
		sscanf(theSessionMap[INIKEY_SE_DTR_FLOW].c_str(), "%x", &isDtr);
		conf_set_int(theCfg, CONF_serflow, isXon ? SER_FLOW_XONXOFF
			: isRts ? SER_FLOW_RTSCTS
			: isDtr ? SER_FLOW_DSRDTR
			: SER_FLOW_NONE);
		return;
	}
	conf_set_str(theCfg, CONF_host, theSessionMap[INIKEY_HOSTNAME].c_str());
	
	int port = 0;
	sscanf(theSessionMap[INIKEY_PROTOCOL] == "SSH2" ? theSessionMap[INIKEY_SSH2PORT].c_str() 
		: theSessionMap[INIKEY_PROTOCOL] == "RLogin" ? "00201"  
		: theSessionMap[INIKEY_PROTOCOL] == "Telnet" ? theSessionMap[INIKEY_PORT].c_str()  
		: theSessionMap[INIKEY_PROTOCOL] == "SSH1" ? theSessionMap[INIKEY_SSH1PORT].c_str()  
		: theSessionMap[INIKEY_SSH2PORT].c_str()
		, "%x", &port);
	conf_set_int(theCfg, CONF_port, port);
	
	std::string username = theSessionMap[INIKEY_USERNAME];
	if (!username.empty()){
		conf_set_int_int(theCfg, CONF_autocmd_enable, 0, 1);
		conf_set_int_str(theCfg, CONF_autocmd, 0, username.c_str());
	}

	int useGlobalKey = 0;
	sscanf(theSessionMap[INIKEY_USEGKEY].c_str(), "%x", &useGlobalKey);
	Filename* fn = filename_from_str(useGlobalKey ? theGlobalMap[INIKEY_PRIKEY].c_str() : theSessionMap[INIKEY_PRIKEY].c_str());
	conf_set_filename(theCfg, CONF_keyfile, fn);
	if (strlen(fn->path) > 0) 
		conf_set_int( theCfg, CONF_agentfwd, 1);
	filename_free(fn);
}

void get_SecureCRT_path_from_lnk(std::string &config_path)
{
	USES_CONVERSION;
	FilePath rootPath(A2W(conf_get_str( cfg, CONF_default_log_path)));
	const FilePath::StringType pattern(L"*SecureCRT*.lnk");
	base::FileEnumerator fileEnumDestop(
		rootPath, false, base::FileEnumerator::FILES,
		pattern);
	FilePath path = fileEnumDestop.Next(); 
	if (path.empty()){
		rootPath = rootPath.Append(L"\\..\\AppData\\Roaming\\Microsoft\\Internet Explorer\\Quick Launch");
		base::FileEnumerator fileEnumQuickLaunch(
			rootPath, false, base::FileEnumerator::FILES,
			pattern);
		path = fileEnumQuickLaunch.Next(); 
		if (path.empty()){
			rootPath = rootPath.Append(L"\\User Pinned\\TaskBar");
			base::FileEnumerator fileEnumQuickLaunchUserPin(
				rootPath, true, base::FileEnumerator::FILES,
				pattern);
			path = fileEnumQuickLaunchUserPin.Next(); 
		}
	}
	if (path.empty()){
		return;
	}
	char secureCRTPath[256] = {0};
	if (0 != read_lnk(W2A(path.value().c_str()), secureCRTPath, sizeof(secureCRTPath)))
		return;
	config_path = std::string(secureCRTPath) + "\\Config";
}

char* winreg_user_read_setting_s(const char* path, const char* key, char*buffer, int buflen);
void get_SecureCRT_path_from_reg(std::string &config_path)
{
	char buf[256] = {0};
	if (NULL == winreg_user_read_setting_s("Software\\VanDyke\\SecureCRT", "Config Path", buf, sizeof(buf)))
		return;
	config_path = std::string(buf);
}
void load_SecureCRT_data(const std::string& config_path)
{
	USES_CONVERSION;
	//get global setting 
	IniMap globalMap;
	globalMap[INIKEY_PRIKEY] = "";
	std::string globalConfigFile = config_path + "\\SSH2.ini";
	parse_ini_file(globalConfigFile.c_str(), globalMap);
	
	//get session setting
	{
		FilePath sessionPath(A2W(config_path.c_str()));
		
		sessionPath = sessionPath.Append(L"\\Sessions");
		base::FileEnumerator sessionFile(
			sessionPath, true, base::FileEnumerator::FILES);
		for (FilePath  path = sessionFile.Next(); !path.empty(); path = sessionFile.Next()){
			if (path.Extension() != L".ini") continue;
			IniMap sessionMap = SESSION_MAP_TMPL;
			parse_ini_file(W2A(path.value().c_str()), sessionMap);	
			if (sessionMap[INIKEY_HOSTNAME].empty()
				|| sessionMap[INIKEY_SSH2PORT].empty())
				continue;
			FilePath::StringType session_name = path.RemoveExtension().value().substr(sessionPath.value().length()+1);
			FilePath::StringType::iterator it = session_name.begin();
			for (; it != session_name.end(); it++) if (*it == L'\\') *it = L'#';
			session_name = FilePath::StringType(A2W(OTHER_SESSION_NAME)) + session_name;
			load_settings(W2A(session_name.c_str()), cfg);
			convertSecureCRTData(globalMap, sessionMap, cfg);
			save_settings(W2A(session_name.c_str()), cfg);

		}
	}
}
void load_sessions_from_SecureCRT()
{
	//find secure crt path
	{
		std::string lnk_config_path;
		get_SecureCRT_path_from_lnk(lnk_config_path);
		if (lnk_config_path.empty())
			return;
		load_SecureCRT_data(lnk_config_path);
	}
	{
		std::string reg_config_path;
		get_SecureCRT_path_from_reg(reg_config_path);
		if (reg_config_path.empty())
			return;
		load_SecureCRT_data(reg_config_path);
	}
}

/**
 * return if need to reload session
 */
int load_sessions_from_others(struct sesslist* sesslist)
{
	/* check if to session exists */
	int first;
	int sessexist;
	first = lower_bound_in_sesslist(sesslist, OTHER_SESSION_NAME);
	sessexist = first >= sesslist->nsessions ? FALSE  
		:(!strncmp(sesslist->sessions[first], OTHER_SESSION_NAME, strlen(OTHER_SESSION_NAME)));
	if (sessexist) {
		return FALSE;
	}

    load_settings(OTHER_SESSION_NAME, cfg);
	save_settings(OTHER_SESSION_NAME, cfg);
	load_sessions_from_SecureCRT();
	return TRUE;
}

#include "base/message_loop.h"
void queue_toplevel_callback(toplevel_callback_fn_t fn, void *ctx)
{
	MessageLoop::current()->PostNonNestableTask(NewRunnableFunction(fn, ctx));
    //struct callback *cb;
	//
    //cb = snew(struct callback);
    //cb->fn = fn;
    //cb->ctx = ctx;
	//
    ///* If the front end has requested notification of pending
    // * callbacks, and we didn't already have one queued, let it know
    // * we do have one now. */
    //if (notify_frontend && !cbhead)
    //    notify_frontend(frontend);
	//
    //if (cbtail)
    //    cbtail->next = cb;
    //else
    //    cbhead = cb;
    //cbtail = cb;
    //cb->next = NULL;
}

void process_in_ui_msg_loop(boost::function<void(void)> func)
{
	WindowInterface::GetInstance()->process_in_msg_loop(func);
}

void register_atexit(void* key, std::function<void()> cb){
	WindowInterface::GetInstance()->register_atexit(key, cb);
}

bool remove_atexit(void* key){
	return WindowInterface::GetInstance()->remove_atexit(key);
}

bool if_need_cmd_scat()
{ 
	return WindowInterface::GetInstance()->ifNeedCmdScat();
}

void cmd_scat(int type, const char * buffer, int buflen, int interactive)
{
	return WindowInterface::GetInstance()->cmdScat(type, buffer, buflen, interactive);
}

void push_wait_open_session(const char* session_name)
{
	return WindowInterface::GetInstance()->push_wait_open_session(session_name);
}

void pop_wait_open_session(char* session_name, int len)
{
	std::string name = WindowInterface::GetInstance()->pop_wait_open_session();
	strncpy(session_name, name.c_str(), len);
}

void schedule_open_wait_sessions(int microseconds);
void open_wait_sessions(void* arg)
{
	std::string name = WindowInterface::GetInstance()->pop_wait_open_session();
	if (name.empty()){ return; }
	if (BrowserList::GetLastActive() == NULL){
		schedule_open_wait_sessions(20000);
		return;
	}
	Conf* backup_cfg = conf_copy(cfg);
	load_settings(name.c_str(), cfg);
	WindowInterface::GetInstance()->createNewSessionWithGlobalCfg();
	conf_copy_into(cfg, backup_cfg);
	conf_free(backup_cfg);
	schedule_open_wait_sessions(10000);
}

void schedule_open_wait_sessions(int microseconds)
{
	struct timeval timeout;
	timeout.tv_sec = microseconds/1000000;
	timeout.tv_usec = microseconds % 1000000;

	g_ui_processor->addLocalTimer(timeout, open_wait_sessions, NULL);
}

void add_to_jumplist_timeout(void* arg)
{
	char* sessionname = (char*)arg;
	add_session_to_jumplist(sessionname);
	sfree(sessionname);
}

void add_to_jumplist_in_bg(const char* sessionname)
{
	char* str = dupstr(sessionname);
	g_bg_processor->process(0, NEW_PROCESSOR_JOB(add_session_to_jumplist, str));
}

void remove_from_jumplist_timeout(void* arg)
{
	char* sessionname = (char*)arg;
	remove_session_from_jumplist(sessionname);
	sfree(sessionname);
}

void remove_from_jumplist_in_bg(const char* sessionname)
{
	char* str = dupstr(sessionname);
	g_bg_processor->process(0, NEW_PROCESSOR_JOB(remove_from_jumplist_timeout, str));
}

HWND get_top_win()
{
	extern HWND hConfigWnd;
	return hConfigWnd != NULL ? hConfigWnd : WindowInterface::GetInstance()->getNativeTopWnd();
}

void update_toolbar(bool should_restore_state)
{
	WindowInterface::GetInstance()->UpdateToolbar(should_restore_state);
}

typedef unsigned long long uint64_t;
uint64_t get_ms()
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;
	return time / 10 / 1000;
}
