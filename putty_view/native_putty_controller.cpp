#include "view/view.h"
#include "view/widget/widget.h"
#include "view/focus/focus_manager.h"
#include "tab_strip.h"

#include "window_interface.h"
#include "native_putty_controller.h"
#include "native_putty_page.h"
#include "native_putty_common.h"
#include "CmdLineHandler.h"
#include "putty_global_config.h"

#include "terminal.h"
#include "storage.h"

#include "atlconv.h" 

#include "Mmsystem.h"

#include "zmodem_session.h"
#include "PuttyFileDialog.h"
#include "putty_view.h"

extern int is_session_log_enabled(void *handle);
extern void log_restart(void *handle, Conf *cfg);
extern void log_stop(void *handle, Conf *cfg);

HMENU NativePuttyController::popup_menu = NULL;
int NativePuttyController::kbd_codepage = 0;
base::Lock NativePuttyController::socketTreeLock_;
void add_keyfile(Filename* filename);


NativePuttyController::NativePuttyController(Conf *theCfg, view::View* theView)
{
	USES_CONVERSION;
	zSession_ = new ZmodemSession(this);
	view_ = theView;
	adjust_host(theCfg);
	cfg = conf_copy(theCfg);
	page_ = NULL;
	must_close_tab_ = FALSE;

	set_input_locale(GetKeyboardLayout(0));
	last_mousemove = 0;

	term = NULL;
    hdc = NULL;
    send_raw_mouse = 0;
    wheel_accumulator = 0;
    busy_status = BUSY_NOT;
    compose_state = 0;
    offset_width = offset_height = conf_get_int(cfg, CONF_window_border);
    caret_x = -1; 
    caret_y = -1;
    n_specials = 0;
    specials = NULL;
    specials_menu = NULL;
    extra_width = 25;
    extra_height = 28;
    font_width = 10;
    font_height = 20;
	font_height_by_wheel = 0;
    offset_width = offset_height = conf_get_int(cfg, CONF_window_border);
    lastact = MA_NOTHING;
    lastbtn = MBT_NOTHING;
    dbltime = GetDoubleClickTime();
    offset_width = conf_get_int(cfg, CONF_window_border);
    offset_height = conf_get_int(cfg, CONF_window_border);
    ignore_clip = FALSE;
    hRgn = NULL;
    hCloserRgn = NULL;
    logbox = NULL;
    nevents = 0;
    negsize = 0;
    events = NULL;
    window_name = icon_name = NULL;
    ldisc = NULL;  
    pal = NULL;
    logpal = NULL;
	closerX = 0;
	closerY = 0;
	char* session_name = conf_get_str(cfg, CONF_session_name);
    char *disrawname = strrchr(session_name, '#');
    disrawname = (disrawname == NULL)? session_name : (disrawname + 1);
    strncpy(disRawName, disrawname, 256);
	disName = A2W(disrawname);
    close_mutex= CreateMutex(NULL, FALSE, NULL);
	backend_state = LOADING;
	isClickingOnPage = false;
	conf_set_int(cfg, CONF_is_enable_shortcut, true);
	next_flash = 0;
	flashing = 0;
	cursor_visible = 1;
	forced_visible = FALSE;
	nativeParentWin_ = NULL;
	isAutoCmdEnabled_ = TRUE;
}

NativePuttyController::~NativePuttyController()
{
	if(checkTimer_.IsRunning()){
        checkTimer_.Stop();
	}
	fini();
	zSession_ = NULL;
	conf_free(cfg);
	expire_timer_context(this);
}

void NativePuttyController::destroy()
{
	zSession_->destroy();
}



UINT NativePuttyController::wm_mousewheel = WM_MOUSEWHEEL;
int NativePuttyController::init(HWND hwndParent)
{
	USES_CONVERSION;
	
	is_frozen = false;
	page_ = new NativePuttyPage();
	page_->init(this, cfg, hwndParent);

    cfgtopalette();

    memset(&ucsdata, 0, sizeof(ucsdata));
    term = term_init(cfg, &ucsdata, this);
    logctx = log_init(this, cfg);
    term_provide_logctx(term, logctx);
    term_size(term, conf_get_int(cfg, CONF_height), 
        conf_get_int(cfg, CONF_width), conf_get_int(cfg, CONF_savelines));   
    init_fonts(0, font_height_by_wheel);

    CreateCaret();
    page_->init_scrollbar(term);
    init_mouse();
	
	Filename* keyfile = conf_get_filename(cfg, CONF_keyfile);
	if (conf_get_int(cfg, CONF_tryagent) && !filename_is_null(keyfile)){
		if (agent_exists()){
			add_keyfile(keyfile);
		}
		else{
			const char* tips = "The pageant is not running.\r\nSo the password of the key, if any, is required for each connection.\r\n";
			from_backend(this, 0, tips, strlen(tips));
		}
	}

    if (start_backend() != 0){
        //MessageBox(WindowInterface::GetInstance()->getNativeTopWnd(), L"failed to start backend!", TEXT("Error"), MB_OK); 
		setDisconnected();
		close_session();
        //return -1;
    }

//	ShowWindow(page_->getWinHandler(), SW_SHOW);
//    SetForegroundWindow(getNativePage());

    init_palette();
    term_set_focus(term, TRUE);
	checkTimer_.Start(
                base::TimeDelta::FromMilliseconds(TIMER_INTERVAL), this,
                &NativePuttyController::checkTimerCallback);


    return 0;
}


void NativePuttyController::checkTimerCallback()
{
	int enabled_auto_reconnect = conf_get_int(cfg, CONF_auto_reconnect);
	if (enabled_auto_reconnect && (must_close_session || must_close_session))
	{
		close_session();
		restartBackend();
		return;
	}
	if (must_close_tab_)
	{
		closeTab();
	}
	if (must_close_session){
		close_session();
	}
}
//-----------------------------------------------------------------------

void NativePuttyController::fini()
{
    int i = 0;
    close_session();

    CloseHandle(close_mutex);
    
    page_->fini();
	delete page_;
    term_free(term);
    term = NULL;
    log_free(logctx);
    logctx = NULL;
    
    deinit_fonts();
    sfree(logpal);
    logpal = NULL;
    sfree(window_name);window_name = NULL;
    sfree(icon_name);icon_name = NULL;
    if (pal)
    	DeleteObject(pal);
    if (conf_get_int(cfg, CONF_protocol) == PROT_SSH) {
    	random_save_seed();
#ifdef MSCRYPTOAPI
    	crypto_wrapup();
#endif
    }
    
    for (i = 0; i < nevents; i++)
        sfree(events[i]);
    sfree(events);

}

//-----------------------------------------------------------------------


/*
 * Copy the colour palette from the configuration data into defpal.
 * This is non-trivial because the colour indices are different.
 */
void NativePuttyController::cfgtopalette()
{
    int i;
    static const int ww[] = {
	256, 257, 258, 259, 260, 261,
	0, 8, 1, 9, 2, 10, 3, 11,
	4, 12, 5, 13, 6, 14, 7, 15
    };

    for (i = 0; i < 22; i++) {
	int w = ww[i];
	defpal[w].rgbtRed = conf_get_int_int(cfg, CONF_colours, i*3+0);
	defpal[w].rgbtGreen = conf_get_int_int(cfg, CONF_colours, i*3+1);
	defpal[w].rgbtBlue = conf_get_int_int(cfg, CONF_colours, i*3+2);
    }
    for (i = 0; i < NEXTCOLOURS; i++) {
	if (i < 216) {
	    int r = i / 36, g = (i / 6) % 6, b = i % 6;
	    defpal[i+16].rgbtRed = r ? r * 40 + 55 : 0;
	    defpal[i+16].rgbtGreen = g ? g * 40 + 55 : 0;
	    defpal[i+16].rgbtBlue = b ? b * 40 + 55 : 0;
	} else {
	    int shade = i - 216;
	    shade = shade * 10 + 8;
	    defpal[i+16].rgbtRed = defpal[i+16].rgbtGreen =
		defpal[i+16].rgbtBlue = shade;
	}
    }

    /* Override with system colours if appropriate */
    if (conf_get_int(cfg, CONF_system_colour))
        systopalette();
}

//-----------------------------------------------------------------------

/*
 * Override bit of defpal with colours from the system.
 * (NB that this takes a copy the system colours at the time this is called,
 * so subsequent colour scheme changes don't take effect. To fix that we'd
 * probably want to be using GetSysColorBrush() and the like.)
 */
void NativePuttyController::systopalette()
{
    int i;
    static const struct { int nIndex; int norm; int bold; } OR[] =
    {
	{ COLOR_WINDOWTEXT,	256, 257 }, /* Default Foreground */
	{ COLOR_WINDOW,		258, 259 }, /* Default Background */
	{ COLOR_HIGHLIGHTTEXT,	260, 260 }, /* Cursor Text */
	{ COLOR_HIGHLIGHT,	261, 261 }, /* Cursor Colour */
    };

    for (i = 0; i < (sizeof(OR)/sizeof(OR[0])); i++) {
    	COLORREF colour = GetSysColor(OR[i].nIndex);
    	defpal[OR[i].norm].rgbtRed =
    	   defpal[OR[i].bold].rgbtRed = GetRValue(colour);
    	defpal[OR[i].norm].rgbtGreen =
    	   defpal[OR[i].bold].rgbtGreen = GetGValue(colour);
    	defpal[OR[i].norm].rgbtBlue =
    	   defpal[OR[i].bold].rgbtBlue = GetBValue(colour);
    }
}

//-----------------------------------------------------------------------

void NativePuttyController::init_fonts(const int pick_width, const int pick_height)
{
	USES_CONVERSION;
    TEXTMETRIC tm;
    CPINFO cpinfo;
    FontSpec *font;
    int fontsize[3];
    int i;
    int quality;
    HDC hdc;
    int fw_dontcare, fw_bold;

    for (i = 0; i < FONT_MAXNO; i++)
	fonts[i] = NULL;

    bold_font_mode = conf_get_int(cfg, CONF_bold_style) & 1 ?
	BOLD_FONT : BOLD_NONE;
    bold_colours = conf_get_int(cfg, CONF_bold_style) & 2 ? TRUE : FALSE;
    und_mode = UND_FONT;

    font = conf_get_fontspec(cfg, CONF_font);
    if (font->isbold) {
	fw_dontcare = FW_BOLD;
	fw_bold = FW_HEAVY;
    } else {
	fw_dontcare = FW_DONTCARE;
	fw_bold = FW_BOLD;
    }

    hdc = GetDC(page_->getWinHandler());

    if (pick_height)
	font_height = pick_height;
    else {
	font_height = font->height;
	if (font_height > 0) {
	    font_height =
		-MulDiv(font_height, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	}
    }
    font_width = pick_width;

    quality = conf_get_int(cfg, CONF_font_quality);
#define f(i,c,w,u) \
    fonts[i] = CreateFont (font_height, font_width, 0, 0, w, FALSE, u, FALSE, \
			   c, OUT_DEFAULT_PRECIS, \
		           CLIP_DEFAULT_PRECIS, FONT_QUALITY(quality), \
			   FIXED_PITCH | FF_DONTCARE, A2W(font->name))

    f(FONT_NORMAL, font->charset, fw_dontcare, FALSE);

    SelectObject(hdc, fonts[FONT_NORMAL]);
    GetTextMetrics(hdc, &tm);

    GetObject(fonts[FONT_NORMAL], sizeof(LOGFONT), &lfont);

    /* Note that the TMPF_FIXED_PITCH bit is defined upside down :-( */
    if (!(tm.tmPitchAndFamily & TMPF_FIXED_PITCH)) {
        font_varpitch = FALSE;
        font_dualwidth = (tm.tmAveCharWidth != tm.tmMaxCharWidth);
    } else {
        font_varpitch = TRUE;
        font_dualwidth = TRUE;
    }
    if (pick_width == 0 || pick_height == 0) {
	font_height = tm.tmHeight;
        font_width = get_font_width(hdc, &tm);
    }

#ifdef RDB_DEBUG_PATCH
    debug(23, "Primary font H=%d, AW=%d, MW=%d",
	    tm.tmHeight, tm.tmAveCharWidth, tm.tmMaxCharWidth);
#endif

    {
	CHARSETINFO info;
	DWORD cset = tm.tmCharSet;
	memset(&info, 0xFF, sizeof(info));

	/* !!! Yes the next line is right */
	if (cset == OEM_CHARSET)
	    ucsdata.font_codepage = GetOEMCP();
	else
	    if (TranslateCharsetInfo ((DWORD *) cset, &info, TCI_SRCCHARSET))
		ucsdata.font_codepage = info.ciACP;
	else
	    ucsdata.font_codepage = -1;

	GetCPInfo(ucsdata.font_codepage, &cpinfo);
	ucsdata.dbcs_screenfont = (cpinfo.MaxCharSize > 1);
    }

    f(FONT_UNDERLINE, font->charset, fw_dontcare, TRUE);

    /*
     * Some fonts, e.g. 9-pt Courier, draw their underlines
     * outside their character cell. We successfully prevent
     * screen corruption by clipping the text output, but then
     * we lose the underline completely. Here we try to work
     * out whether this is such a font, and if it is, we set a
     * flag that causes underlines to be drawn by hand.
     *
     * Having tried other more sophisticated approaches (such
     * as examining the TEXTMETRIC structure or requesting the
     * height of a string), I think we'll do this the brute
     * force way: we create a small bitmap, draw an underlined
     * space on it, and test to see whether any pixels are
     * foreground-coloured. (Since we expect the underline to
     * go all the way across the character cell, we only search
     * down a single column of the bitmap, half way across.)
     */
    {
	HDC und_dc;
	HBITMAP und_bm, und_oldbm;
	int i, gotit;
	COLORREF c;

	und_dc = CreateCompatibleDC(hdc);
	und_bm = CreateCompatibleBitmap(hdc, font_width, font_height);
	und_oldbm = (HBITMAP__*)SelectObject(und_dc, und_bm);
	SelectObject(und_dc, fonts[FONT_UNDERLINE]);
	SetTextAlign(und_dc, TA_TOP | TA_LEFT | TA_NOUPDATECP);
	SetTextColor(und_dc, RGB(255, 255, 255));
	SetBkColor(und_dc, RGB(0, 0, 0));
	SetBkMode(und_dc, OPAQUE);
	ExtTextOut(und_dc, 0, 0, ETO_OPAQUE, NULL, A2W(" "), 1, NULL);
	gotit = FALSE;
	for (i = 0; i < font_height; i++) {
	    c = GetPixel(und_dc, font_width / 2, i);
	    if (c != RGB(0, 0, 0))
		gotit = TRUE;
	}
	SelectObject(und_dc, und_oldbm);
	DeleteObject(und_bm);
	DeleteDC(und_dc);
	if (!gotit) {
	    und_mode = UND_LINE;
	    DeleteObject(fonts[FONT_UNDERLINE]);
	    fonts[FONT_UNDERLINE] = 0;
	}
    }

    if (bold_font_mode == BOLD_FONT) {
	f(FONT_BOLD, font->charset, fw_bold, FALSE);
    }
#undef f

    descent = tm.tmAscent + 1;
    if (descent >= font_height)
	descent = font_height - 1;

    for (i = 0; i < 3; i++) {
	if (fonts[i]) {
	    if (SelectObject(hdc, fonts[i]) && GetTextMetrics(hdc, &tm))
		fontsize[i] = get_font_width(hdc, &tm) + 256 * tm.tmHeight;
	    else
		fontsize[i] = -i;
	} else
	    fontsize[i] = -i;
    }

    ReleaseDC(page_->getWinHandler(), hdc);

    if (fontsize[FONT_UNDERLINE] != fontsize[FONT_NORMAL]) {
	und_mode = UND_LINE;
	DeleteObject(fonts[FONT_UNDERLINE]);
	fonts[FONT_UNDERLINE] = 0;
    }

    if (bold_font_mode == BOLD_FONT &&
	fontsize[FONT_BOLD] != fontsize[FONT_NORMAL]) {
	bold_font_mode = BOLD_SHADOW;
	DeleteObject(fonts[FONT_BOLD]);
	fonts[FONT_BOLD] = 0;
    }
    fontflag[0] = fontflag[1] = fontflag[2] = 1;

    init_ucs(cfg, &ucsdata);
}

void NativePuttyController::deinit_fonts()
{
    int i;
    for (i = 0; i < FONT_MAXNO; i++) {
    	if (fonts[i])
    	    DeleteObject(fonts[i]);
    	fonts[i] = 0;
    	fontflag[i] = 0;
    }
}
//-----------------------------------------------------------------------

int NativePuttyController::get_font_width(HDC hdc, const TEXTMETRIC *tm)
{
    int ret;
    /* Note that the TMPF_FIXED_PITCH bit is defined upside down :-( */
    if (!(tm->tmPitchAndFamily & TMPF_FIXED_PITCH)) {
        ret = tm->tmAveCharWidth;
    } else {
#define FIRST '0'
#define LAST '9'
        ABCFLOAT widths[LAST-FIRST + 1];
        int j;

        font_varpitch = TRUE;
        font_dualwidth = TRUE;
        if (GetCharABCWidthsFloat(hdc, FIRST, LAST, widths)) {
            ret = 0;
            for (j = 0; j < lenof(widths); j++) {
                int width = (int)(0.5 + widths[j].abcfA +
                                  widths[j].abcfB + widths[j].abcfC);
                if (ret < width)
                    ret = width;
            }
        } else {
            ret = tm->tmMaxCharWidth;
        }
#undef FIRST
#undef LAST
    }
    return ret;
}

//-----------------------------------------------------------------------

int NativePuttyController::CreateCaret()
{
    /*
     * Set up a caret bitmap, with no content.
     */
	char *bits;
	int size = (font_width + 15) / 16 * 2 * font_height;
	bits = snewn(size, char);
	memset(bits, 0, size);
	caretbm = CreateBitmap(font_width, font_height, 1, 1, bits);
	sfree(bits);

    ::CreateCaret(page_->hwndCtrl, caretbm, 
        font_width, font_height);
    return 0;
}
//-----------------------------------------------------------------------

int NativePuttyController::init_mouse()
{
    lastact = MA_NOTHING;
    lastbtn = MBT_NOTHING;
    dbltime = GetDoubleClickTime();
    return 0;
}

//-----------------------------------------------------------------------

int NativePuttyController::start_backend()
{
	 USES_CONVERSION;;
    const char *error;
	char msg[2048] = { 0 };
    char *realhost = NULL; 
    /*
     * Select protocol. This is farmed out into a table in a
     * separate file to enable an ssh-free variant.
     */
    back = backend_from_proto(conf_get_int(cfg, CONF_protocol));
    if (back == NULL) {
		const char* str = "\r\n"
			"===============================================================\r\n"
			"--------         Unsupported protocol number found      -------\r\n"
			"===============================================================\r\n";
		term_data(term, 1, str, strlen(str));
	    return -1;
    }

    error = back->init(this, &backhandle, cfg,
		       conf_get_str(cfg, CONF_host), conf_get_int(cfg, CONF_port), &realhost, conf_get_int(cfg, CONF_tcp_nodelay),
		       conf_get_int(cfg, CONF_tcp_keepalives));
    back->provide_logctx(backhandle, logctx);
    if (error) {
    	snprintf(msg, sizeof(msg) - 1,"\r\n"
			"===============================================================\r\n"
			"--------    Unable to open connection to%.800s\r\n" 
			"--------Error:%s\r\n"
			"===============================================================\r\n"
			, conf_get_str( cfg, CONF_session_name), error);
		term_data(term, 1, msg, strlen(msg));
	    return -1;
    }

    if (realhost != NULL) sfree(realhost);

    /*
     * Connect the terminal to the backend for resize purposes.
     */
    term_provide_resize_fn(term, back->size, backhandle);

    /*
     * Set up a line discipline.
     */
    ldisc = ldisc_create(cfg, term
                    , back, backhandle, this);
	int protocol = conf_get_int(cfg, CONF_protocol);
	if (PROT_SERIAL == protocol || PROT_ADB == protocol){
		setConnected();
	}
    must_close_session = FALSE;
    session_closed = FALSE;
    return 0;
}

//-----------------------------------------------------------------------

void NativePuttyController::init_palette()
{
    int i;
    HDC hdc = GetDC(page_->getWinHandler());
    if (hdc) {
	if (conf_get_int(cfg, CONF_try_palette) && GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) {
	    /*
	     * This is a genuine case where we must use smalloc
	     * because the snew macros can't cope.
	     */
	    logpal = (tagLOGPALETTE*)smalloc(sizeof(*logpal)
			     - sizeof(logpal->palPalEntry)
			     + NALLCOLOURS * sizeof(PALETTEENTRY));
	    logpal->palVersion = 0x300;
	    logpal->palNumEntries = NALLCOLOURS;
	    for (i = 0; i < NALLCOLOURS; i++) {
    		logpal->palPalEntry[i].peRed = defpal[i].rgbtRed;
    		logpal->palPalEntry[i].peGreen = defpal[i].rgbtGreen;
    		logpal->palPalEntry[i].peBlue = defpal[i].rgbtBlue;
    		logpal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
	    }
	    pal = CreatePalette(logpal);
	    if (pal) {
    		SelectPalette(hdc, pal, FALSE);
    		RealizePalette(hdc);
    		SelectPalette(hdc, (HPALETTE__*)GetStockObject(DEFAULT_PALETTE), FALSE);
	    }
	}
	ReleaseDC(page_->getWinHandler(), hdc);
    }
    if (pal){
    	for (i = 0; i < NALLCOLOURS; i++)
    	    colours[i] = PALETTERGB(defpal[i].rgbtRed,
    				    defpal[i].rgbtGreen,
    				    defpal[i].rgbtBlue);
    } else {
    	for (i = 0; i < NALLCOLOURS; i++)
    	    colours[i] = RGB(defpal[i].rgbtRed,
    			     defpal[i].rgbtGreen, defpal[i].rgbtBlue);
    }
}

//-----------------------------------------------------------------------

void NativePuttyController::close_session()
{
    must_close_session = FALSE;
    
    if (!back)  return;
    
    DWORD wait_result = WaitForSingleObject(close_mutex, INFINITE);
    if (WAIT_OBJECT_0 != wait_result)
        return;

    if (!back)  {
        ReleaseMutex(close_mutex);
        return;
    }
    //char morestuff[100];
    //int i;


    //sprintf(morestuff, "%.70s (inactive)", appname);
    //set_icon(NULL, morestuff);
    //set_title(NULL, morestuff);
    
    
    session_closed = TRUE;
    if (ldisc) {
    	ldisc_free(ldisc);
    	ldisc = NULL;
		term->ldisc = NULL;
    }
    if (back) {
    	back->free(backhandle);
    	backhandle = NULL;
    	back = NULL;
        term_provide_resize_fn(term, NULL, NULL);
        update_specials_menu();
	
    }

    /*
     * Show the Restart Session menu item. Do a precautionary
     * delete first to ensure we never end up with more than one.
     */
    //for (i = 0; i < lenof(popup_menus); i++) {
	//DeleteMenu(popup_menus[i].menu, IDM_RESTART, MF_BYCOMMAND);
	//InsertMenu(popup_menus[i].menu, IDM_DUPSESS, MF_BYCOMMAND | MF_ENABLED,
	//	   IDM_RESTART, "&Restart Session");
    //}
    ReleaseMutex(close_mutex);
}

//-----------------------------------------------------------------------

/*
 * Update the Special Commands submenu.
 */
void NativePuttyController::update_specials_menu()
{
	USES_CONVERSION;
    HMENU new_menu;
    int i;//, j;

    if (back)
	specials = back->get_specials(backhandle);
    else
	specials = NULL;

    if (specials) {
	/* We can't use Windows to provide a stack for submenus, so
	 * here's a lame "stack" that will do for now. */
	HMENU saved_menu = NULL;
	int nesting = 1;
	new_menu = CreatePopupMenu();
	for (i = 0; nesting > 0; i++) {
	    assert(IDM_SPECIAL_MIN + 0x10 * i < IDM_SPECIAL_MAX);
	    switch (specials[i].code) {
	      case TS_SEP:
		AppendMenu(new_menu, MF_SEPARATOR, 0, 0);
		break;
	      case TS_SUBMENU:
		assert(nesting < 2);
		nesting++;
		saved_menu = new_menu; /* XXX lame stacking */
		new_menu = CreatePopupMenu();
		AppendMenu(saved_menu, MF_POPUP | MF_ENABLED,
			   (UINT) new_menu, A2W(specials[i].name));
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
			   A2W(specials[i].name));
		break;
	    }
	}
	/* Squirrel the highest special. */
	n_specials = i - 1;
    } else {
	new_menu = NULL;
	n_specials = 0;
    }
    /* not show the special menu 
    for (j = 0; j < lenof(popup_menus); j++) {
	if (specials_menu) {
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
    specials_menu = new_menu;
}

/*
void NativePuttyController::require_resize(int term_width, int term_height)
{
    int page_width = term_width + page.extra_page_width;
    int page_height = term_height + page.extra_page_height;
    int parent_width = page_width + page.extra_width;
    int parent_height = page_height + page.extra_height;
    
    wintab_require_resize((wintab*)parentTab, parent_width, parent_height);

    SetWindowPos(getNativePage(), NULL, 0, 0, 
        page_width, page_height, SWP_NOMOVE | SWP_NOZORDER); 
    //MoveWindow(getNativePage(), rc->left, rc->top, 
    //    rc->right - rc->left, 
    //    rc->bottom - rc->top, TRUE);
}

//-----------------------------------------------------------------------

void NativePuttyController::get_extra_size(int *extra_width, int *extra_height)
{
    wintab_get_extra_size((wintab*)parentTab, extra_width, extra_height);
    *extra_width += page.extra_page_width + page.extra_width;
    *extra_height += page.extra_page_height + page.extra_height;
}
*/
//-----------------------------------------------------------------------

int NativePuttyController::can_close()
{
    if (conf_get_int(cfg, CONF_warn_on_close) && !session_closed)
        return FALSE;
    return TRUE;
}


//-----------------------------------------------------------------------

int NativePuttyController::on_scroll(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam)) {
	  case SB_BOTTOM:
	    term_scroll(term, -1, 0);
	    break;
	  case SB_TOP:
	    term_scroll(term, +1, 0);
	    break;
	  case SB_LINEDOWN:
	    term_scroll(term, 0, +1);
	    break;
	  case SB_LINEUP:
	    term_scroll(term, 0, -1);
	    break;
	  case SB_PAGEDOWN:
	    term_scroll(term, 0, +term->rows / 2);
	    break;
	  case SB_PAGEUP:
	    term_scroll(term, 0, -term->rows / 2);
	    break;
	  case SB_THUMBPOSITION:
	  case SB_THUMBTRACK:
	    term_scroll(term, 1, HIWORD(wParam));
	    break;
	}
    return 0;
}

//-----------------------------------------------------------------------

int NativePuttyController::on_paint()
{
    PAINTSTRUCT p;

	HideCaret(page_->getWinHandler());
    hdc = BeginPaint(page_->getWinHandler(), &p);
    if (pal) {
    	SelectPalette(hdc, pal, TRUE);
    	RealizePalette(hdc);
    }

    /*
     * We have to be careful about term_paint(). It will
     * set a bunch of character cells to INVALID and then
     * call do_paint(), which will redraw those cells and
     * _then mark them as done_. This may not be accurate:
     * when painting in WM_PAINT context we are restricted
     * to the rectangle which has just been exposed - so if
     * that only covers _part_ of a character cell and the
     * rest of it was already visible, that remainder will
     * not be redrawn at all. Accordingly, we must not
     * paint any character cell in a WM_PAINT context which
     * already has a pending update due to terminal output.
     * The simplest solution to this - and many, many
     * thanks to Hung-Te Lin for working all this out - is
     * not to do any actual painting at _all_ if there's a
     * pending terminal update: just mark the relevant
     * character cells as INVALID and wait for the
     * scheduled full update to sort it out.
     * 
     * I have a suspicion this isn't the _right_ solution.
     * An alternative approach would be to have terminal.c
     * separately track what _should_ be on the terminal
     * screen and what _is_ on the terminal screen, and
     * have two completely different types of redraw (one
     * for full updates, which syncs the former with the
     * terminal itself, and one for WM_PAINT which syncs
     * the latter with the former); yet another possibility
     * would be to have the Windows front end do what the
     * GTK one already does, and maintain a bitmap of the
     * current terminal appearance so that WM_PAINT becomes
     * completely trivial. However, this should do for now.
     */
    term_paint(term, (Context)this, 
	       (p.rcPaint.left-offset_width)/font_width,
	       (p.rcPaint.top-offset_height)/font_height,
	       (p.rcPaint.right-offset_width-1)/font_width,
	       (p.rcPaint.bottom-offset_height-1)/font_height,
	       !term->window_update_pending);

    if (p.fErase ||
        p.rcPaint.left  < offset_width  ||
	p.rcPaint.top   < offset_height ||
	p.rcPaint.right >= offset_width + font_width*term->cols ||
	p.rcPaint.bottom>= offset_height + font_height*term->rows)
    {
    	HBRUSH fillcolour, oldbrush;
    	HPEN   edge, oldpen;
    	fillcolour = CreateSolidBrush (
    			    colours[ATTR_DEFBG>>ATTR_BGSHIFT]);
    	oldbrush = (HBRUSH__*)SelectObject(hdc, fillcolour);
    	edge = CreatePen(PS_SOLID, 0, 
    			    colours[ATTR_DEFBG>>ATTR_BGSHIFT]);
    	oldpen = (HPEN__*)SelectObject(hdc, edge);

    	/*
    	 * Jordan Russell reports that this apparently
    	 * ineffectual IntersectClipRect() call masks a
    	 * Windows NT/2K bug causing strange display
    	 * problems when the PuTTY window is taller than
    	 * the primary monitor. It seems harmless enough...
    	 */
    	IntersectClipRect(hdc,
    		p.rcPaint.left, p.rcPaint.top,
    		p.rcPaint.right, p.rcPaint.bottom);

    	ExcludeClipRect(hdc, 
    		offset_width, offset_height,
    		offset_width+font_width*term->cols,
    		offset_height+font_height*term->rows);

    	Rectangle(hdc, p.rcPaint.left, p.rcPaint.top, 
    		  p.rcPaint.right, p.rcPaint.bottom);

    	/* SelectClipRgn(hdc, NULL); */

    	SelectObject(hdc, oldbrush);
    	DeleteObject(fillcolour);
    	SelectObject(hdc, oldpen);
    	DeleteObject(edge);
    }
    SelectObject(hdc, GetStockObject(SYSTEM_FONT));
    SelectObject(hdc, GetStockObject(WHITE_PEN));
    EndPaint(page_->getWinHandler(), &p);
    ShowCaret(page_->getWinHandler());

    return 0;
}

//-----------------------------------------------------------------------

int NativePuttyController::swallow_shortcut_key(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_KEYDOWN && message != WM_SYSKEYDOWN)
		return 0;

	if (!PuttyGlobalConfig::GetInstance()->isShotcutKeyEnabled())
		return 0;

	BYTE keystate[256];
	if (GetKeyboardState(keystate) == 0)
		return 0;

	int ctrl_pressed = (keystate[VK_CONTROL] & 0x80);
	int shift_pressed = (keystate[VK_SHIFT] & 0x80);
	int alt_pressed = (keystate[VK_MENU] & 0x80);

	if (zSession_->isDoingRz()){
		//const std::string err("It has been considered to terminate the session with Ctrl+C. But it is troublesome");
		if (!alt_pressed && ctrl_pressed && !shift_pressed && wParam == 'C'){
			zSession_->reset();
			const std::string promp("\r\nuser cancelled, if you are doing rz, please wait until the reset package is sent in buffer.\r\n");
			term_data(term, 0, promp.c_str(), promp.length());
		}
		return 1;
	}

	int key_type = (wParam >= VK_F1 && wParam <= VK_F12) ? (wParam - VK_F1 + F1)
		: ctrl_pressed && shift_pressed && !alt_pressed ? CTRL_SHIFT
		: ctrl_pressed && !shift_pressed && !alt_pressed ? CTRL
		: !ctrl_pressed && !shift_pressed && alt_pressed ? ALT
		: -1;
	if (key_type == -1){ return 0; }
	const char* rule = PuttyGlobalConfig::GetInstance()->getShortcutRules(key_type, wParam);
	if (rule == NULL){ return 0; }
	if (!strcmp(rule, SHORTCUT_KEY_SELECT_TAB)) {
		if (wParam == '0'){
			WindowInterface::GetInstance()->selectTab(9);
			return 1;
		}
		else if (wParam >= '1' && wParam <= '9'){
			WindowInterface::GetInstance()->selectTab(wParam - '1');
			return 1;
		}
	}
	else if (!strcmp(rule, SHORTCUT_KEY_SELECT_NEXT_TAB)) {
		WindowInterface::GetInstance()->selectNextTab();
		return 1;
	}
	else if (!strcmp(rule, SHORTCUT_KEY_SELECT_PRE_TAB)) {
		WindowInterface::GetInstance()->selectPreviousTab();
		return 1;
	}
	else if (!strcmp(rule, SHORTCUT_KEY_DUP_TAB)){
		WindowInterface::GetInstance()->dupCurSession();
		return 1;
	}
	else if (!strcmp(rule, SHORTCUT_KEY_NEW_TAB)){
		WindowInterface::GetInstance()->createNewSession();
		return 1;
	}
	else if (!strcmp(rule, SHORTCUT_KEY_RELOAD_TAB)){
		WindowInterface::GetInstance()->reloadCurrentSession();
		return 1;
	}
	else if (!strcmp(rule, SHORTCUT_KEY_EDIT_TAB_TITLE)){
		rename();
		return 1;
	}
	else if (!strcmp(rule, SHORTCUT_KEY_RENAME_SESSION)){
		rename();
		return 1;
	}
	else if (!strcmp(rule, SHORTCUT_KEY_HIDE_SHOW_TOOLBAR)){
		hide_toolbar();
		return 1;
	}
	else if (!strcmp(rule, SHORTCUT_KEY_CLOSE_TAB)){
		closeTab();
		return 1;
	}

    return 0;

}
extern Conf* cfg;
int NativePuttyController::on_reconfig()
{
	Conf* prev_cfg;
	int init_lvl = 1;
	int reconfig_result;
   
	//GetWindowText(hwnd, conf_get_int(cfg, CONF_wintitle), sizeof(conf_get_int(cfg, CONF_wintitle)));
	prev_cfg = conf_copy(this->cfg);
    conf_copy_into(::cfg, this->cfg);

	reconfig_result =
	    do_reconfig(getNativePage(), this->back ? this->back->cfg_info(this->backhandle) : 0);
   if (!reconfig_result)
	    return 0;
    
   conf_copy_into(this->cfg, ::cfg);
	//{
	//    /* Disable full-screen if resizing forbidden */
	//    int i;
	//    for (i = 0; i < lenof(popup_menus); i++)
	//	EnableMenuItem(popup_menus[i].menu, IDM_FULLSCREEN,
	//		       MF_BYCOMMAND | 
	//		       (conf_get_int(cfg, CONF_resize_action) == RESIZE_DISABLED)
	//		       ? MF_GRAYED : MF_ENABLED);
	//    /* Gracefully unzoom if necessary */
	//    if (IsZoomed(hwnd) &&
	//	(conf_get_int(cfg, CONF_resize_action) == RESIZE_DISABLED)) {
	//	ShowWindow(hwnd, SW_RESTORE);
	//    }
	//}
	//if (!prev_cfg.no_remote_tabname && conf_get_int(cfg, CONF_no_remote_tabname)){
 //       char *disrawname = strrchr(conf_get_int(this->cfg, CONF_session_name), '#');
 //       disrawname = (disrawname == NULL)? conf_get_int(this->cfg, CONF_session_name) : (disrawname + 1);
 //       strncpy(this->disRawName, disrawname, 256);
	//}

	/* Pass new config data to the logging module */
	log_reconfig(this->logctx, cfg);

	sfree(this->logpal);
	/*
	 * Flush the line discipline's edit buffer in the
	 * case where local editing has just been disabled.
	 */
	if (this->ldisc)
	    ldisc_send(this->ldisc, NULL, 0, 0);
	if (this->pal)
	    DeleteObject(this->pal);
	this->logpal = NULL;
	this->pal = NULL;
	cfgtopalette();
	init_palette();

	/* Pass new config data to the terminal */
	term_reconfig(this->term, this->cfg);

	/* Pass new config data to the back end */
	if (this->back)
	    this->back->reconfig(this->backhandle, this->cfg);

	/* Enable or disable the scroll bar, etc */
	{
	    LONG nflg, flag = GetWindowLongPtr(getNativePage(), GWL_STYLE);
	    LONG nexflag, exflag =
		GetWindowLongPtr(getNativePage(), GWL_EXSTYLE);

	    nexflag = exflag;
        nflg = flag;
	    if (conf_get_int(cfg, CONF_alwaysontop) != conf_get_int(prev_cfg, CONF_alwaysontop)) {
    		if (conf_get_int(cfg, CONF_alwaysontop)) {
    		    nexflag |= WS_EX_TOPMOST;
    		    SetWindowPos(getNativePage(), HWND_TOPMOST, 0, 0, 0, 0,
    				 SWP_NOMOVE | SWP_NOSIZE);
    		} else {
    		    nexflag &= ~(WS_EX_TOPMOST);
    		    SetWindowPos(getNativePage(), HWND_NOTOPMOST, 0, 0, 0, 0,
    				 SWP_NOMOVE | SWP_NOSIZE);
    		}
	    }

	    if (nflg != flag || nexflag != exflag) {
    		if (nflg != flag)
    		    SetWindowLongPtr(getNativePage(), GWL_STYLE, nflg);
    		if (nexflag != exflag)
    		    SetWindowLongPtr(getNativePage(), GWL_EXSTYLE, nexflag);

    		SetWindowPos(getNativePage(), NULL, 0, 0, 0, 0,
    			     SWP_NOACTIVATE | SWP_NOCOPYBITS |
    			     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
    			     SWP_FRAMECHANGED);

    		init_lvl = 2;
	    }
        LONG npflg, pflag = GetWindowLongPtr(getNativePage(), GWL_STYLE);
        npflg = pflag;
	    if (is_full_screen() ?
    		conf_get_int(cfg, CONF_scrollbar_in_fullscreen) : conf_get_int(cfg, CONF_scrollbar))
		npflg |= WS_VSCROLL;
	    else
		npflg &= ~WS_VSCROLL;
        if (npflg != pflag)
		    SetWindowLongPtr(getNativePage(), GWL_STYLE, nflg);
	}

	/* Oops */
	//if (this->conf_get_int(cfg, CONF_resize_action) == RESIZE_DISABLED && IsZoomed(getNativePage())) {
	//    force_normal(hwnd);
	//    init_lvl = 2;
	//}

	////set_title(this, this->conf_get_int(cfg, CONF_wintitle));
	//if (IsIconic(hwnd)) {
	//    SetWindowText(hwnd,
	//		  this->conf_get_int(cfg, CONF_win_name_always) ? this->window_name :
	//		  this->icon_name);
	//}

	FontSpec* new_font = conf_get_fontspec(cfg, CONF_font);
	FontSpec* old_font = conf_get_fontspec(prev_cfg, CONF_font);
	if (strcmp(new_font->name, old_font->name) != 0 ||
		strcmp(conf_get_str(cfg, CONF_line_codepage), conf_get_str(prev_cfg, CONF_line_codepage)) != 0 ||
		new_font->isbold != old_font->isbold ||
		new_font->height != old_font->height ||
		new_font->charset != old_font->charset ||
		conf_get_int(cfg, CONF_font_quality) != conf_get_int(prev_cfg, CONF_font_quality) ||
		conf_get_int(cfg, CONF_vtmode) != conf_get_int(prev_cfg, CONF_vtmode) ||
		conf_get_int(cfg, CONF_bold_style) != conf_get_int(prev_cfg, CONF_bold_style) ||
		conf_get_int(cfg, CONF_resize_action) == RESIZE_DISABLED ||
		conf_get_int(cfg, CONF_resize_action) == RESIZE_EITHER ||
		(conf_get_int(cfg, CONF_resize_action) != conf_get_int(prev_cfg, CONF_resize_action))) {
		font_height_by_wheel = 0;
		init_lvl = 2;
	}

	conf_free(prev_cfg);
	InvalidateRect(getNativePage(), NULL, TRUE);
	reset_window(init_lvl);
	    
    return 0;
}

int NativePuttyController::on_menu( HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
	
    switch (wParam & ~0xF) {       // low 4 bits reserved to Windows 
        case IDM_SHOWLOG:
            showeventlog(this, hwnd);
            break;
        case IDM_START_STOP_LOG:
        {
            if (!is_session_log_enabled(logctx))
            {
    			CheckMenuItem(popup_menu, IDM_START_STOP_LOG, MF_CHECKED);
                log_restart(logctx, cfg);
            }
            else
            {
    			CheckMenuItem(popup_menu, IDM_START_STOP_LOG, MF_UNCHECKED);
    			/* Pass new config data to the logging module */
    		    log_stop(logctx, cfg);
            }
        }
            break;
        case IDM_NEWSESS:
			WindowInterface::GetInstance()->createNewSession();
			break;
        case IDM_DUPSESS:
			WindowInterface::GetInstance()->dupCurSession();
			break;
		case IDM_RECONF:
            on_reconfig();
            break;
        case IDM_RESTART:{
			restartBackend();
            break;
        }
        case IDM_COPYALL:
            term_copyall(term);
            break;
        case IDM_PASTE:
            request_paste();
            break;
        case IDM_CLRSB:
            term_clrsb(term);
            break;
        case IDM_RESET:
            term_pwron(term, TRUE);
			isAutoCmdEnabled_ = TRUE;
            if (ldisc)
            ldisc_send(ldisc, NULL, 0, 0);
            break;
 //       case IDM_ABOUT:
 //           showabout(hwnd);
 //           break;
 //       case IDM_HELP:
 //           launch_help(hwnd, NULL);
 //           break;
 //       case SC_MOUSEMENU:
 //           /*
 //            * We get this if the System menu has been activated
 //            * using the mouse.
 //            */
 //           show_mouseptr(wintab_get_active_item(&tab), 1);
 //           break;
 //       case SC_KEYMENU:
 //           /*
 //            * We get this if the System menu has been activated
 //            * using the keyboard. This might happen from within
 //            * TranslateKey, in which case it really wants to be
 //            * followed by a `space' character to actually _bring
 //            * the menu up_ rather than just sitting there in
 //            * `ready to appear' state.
 //            */
 //           show_mouseptr(wintab_get_active_item(&tab), 1);	       /* make sure pointer is visible */
 //           if( lParam == 0 )
 //               PostMessage(getNativePage(), WM_CHAR, ' ', 0);
 //           break;
 //       case IDM_FULLSCREEN:
 //           flip_full_screen();
 //           break;
        default:
 //           if (wParam >= IDM_SAVED_MIN && wParam < IDM_SAVED_MAX) {
 //               SendMessage(hwnd, WM_SYSCOMMAND, IDM_SAVEDSESS, wParam);
 //           }
 //           if (wParam >= IDM_SPECIAL_MIN && wParam <= IDM_SPECIAL_MAX) {
 //               int i = (wParam - IDM_SPECIAL_MIN) / 0x10;
 //               /*
 //                * Ensure we haven't been sent a bogus SYSCOMMAND
 //                * which would cause us to reference invalid memory
 //                * and crash. Perhaps I'm just too paranoid here.
 //                */
 //               if (i >= n_specials)
 //                   break;
 //               if (back)
 //                   back->special(backhandle, (Telnet_Special)specials[i].code);
 //               net_pending_errors();
 //           }
			break;
	}
    return 0;
}

void NativePuttyController::process_log_status()
{
	//wintab *tab = (wintab *)parentTab;
 //   if (!is_session_log_enabled(logctx))
 //   {
 //       CheckMenuItem(popup_menus[CTXMENU].menu, IDM_START_STOP_LOG, MF_UNCHECKED);
	//	CheckMenuItem(popup_menus[SYSMENU].menu, IDM_START_STOP_LOG, MF_UNCHECKED);
 //       SendMessage(tab->hToolBar, TB_SETSTATE,
 //       			(WPARAM)IDM_START_STOP_LOG, (LPARAM)TBSTATE_ENABLED);
 //   }
 //   else
 //   {
 //       CheckMenuItem(popup_menus[CTXMENU].menu, IDM_START_STOP_LOG, MF_CHECKED);
	//	CheckMenuItem(popup_menus[SYSMENU].menu, IDM_START_STOP_LOG, MF_CHECKED);
 //       SendMessage(tab->hToolBar, TB_SETSTATE,
 //       			(WPARAM)IDM_START_STOP_LOG, (LPARAM)TBSTATE_CHECKED | TBSTATE_ENABLED);
 //   }
}

HWND NativePuttyController::getNativePage(){
	return page_->hwndCtrl;
}

void NativePuttyController::sys_cursor_update()
{
	COMPOSITIONFORM cf;
    HIMC hIMC;

    if (!term->has_focus) return;

    if (caret_x < 0 || caret_y < 0)
	return;

    SetCaretPos(caret_x, caret_y);

    /* IMM calls on Win98 and beyond only */
	OSVERSIONINFO& osVersion = get_os_version();
    if(osVersion.dwPlatformId == VER_PLATFORM_WIN32s) return; /* 3.11 */
    
    if(osVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
	    osVersion.dwMinorVersion == 0) return; /* 95 */

    /* we should have the IMM functions */
    hIMC = ImmGetContext(getNativePage());
    cf.dwStyle = CFS_POINT;
    cf.ptCurrentPos.x = caret_x;
    cf.ptCurrentPos.y = caret_y;
    ImmSetCompositionWindow(hIMC, &cf);

    ImmReleaseContext(getNativePage(), hIMC);
}

void NativePuttyController::update_mouse_pointer()
{
    
    LPTSTR curstype;
    int force_visible = FALSE;
    switch (busy_status) {
      case BUSY_NOT:
	if (send_raw_mouse)
	    curstype = IDC_ARROW;
	else
	    curstype = IDC_IBEAM;
	break;
      case BUSY_WAITING:
	curstype = IDC_APPSTARTING; /* this may be an abuse */
	force_visible = TRUE;
	break;
      case BUSY_CPU:
	curstype = IDC_WAIT;
	force_visible = TRUE;
	break;
      default:
	assert(0);
    }
    {
	HCURSOR cursor = LoadCursor(NULL, curstype);
	SetClassLongPtr(getNativePage(), GCLP_HCURSOR, (LONG_PTR)cursor);
	SetCursor(cursor); /* force redraw of cursor at current posn */
    }
    if (force_visible != forced_visible) {
	/* We want some cursor shapes to be visible always.
	 * Along with show_mouseptr(), this manages the ShowCursor()
	 * counter such that if we switch back to a non-force_visible
	 * cursor, the previous visibility state is restored. */
	ShowCursor(force_visible);
	forced_visible = force_visible;
    }
}

/*
 * Draw a line of text in the window, at given character
 * coordinates, in given attributes.
 *
 * We are allowed to fiddle with the contents of `text'.
 */
void NativePuttyController::do_text_internal(int x, int y, wchar_t *text, int len,
		      unsigned long attr, int lattr)
{
	USES_CONVERSION;
    COLORREF fg, bg, t;
    int nfg, nbg, nfont;
    RECT line_box;
    int force_manual_underline = 0;
    int fnt_width, char_width;
    int text_adjust = 0;
    int xoffset = 0;
    int maxlen, remaining, opaque;
    static int *lpDx = NULL;
    static int lpDx_len = 0;
    int *lpDx_maybe;

    assert (hdc != NULL);

    lattr &= LATTR_MODE;

    char_width = fnt_width = font_width * (1 + (lattr != LATTR_NORM));

    if (attr & ATTR_WIDE)
	char_width *= 2;

    /* Only want the left half of double width lines */
    if (lattr != LATTR_NORM && x*2 >= term->cols)
	return;

    x *= fnt_width;
    y *= font_height;
    x += offset_width;
    y += offset_height;

    if ((attr & TATTR_ACTCURS) && (conf_get_int(cfg, CONF_cursor_type) == 0 || term->big_cursor)) {
	attr &= ~(ATTR_REVERSE|ATTR_BLINK|ATTR_COLOURS);
	if (bold_font_mode == BOLD_NONE)
	    attr &= ~ATTR_BOLD;

	/* cursor fg and bg */
	attr |= (260 << ATTR_FGSHIFT) | (261 << ATTR_BGSHIFT);
    }

    nfont = 0;
    if (conf_get_int(cfg, CONF_vtmode) == VT_POORMAN && lattr != LATTR_NORM) {
	/* Assume a poorman font is borken in other ways too. */
	lattr = LATTR_WIDE;
    } else
	switch (lattr) {
	  case LATTR_NORM:
	    break;
	  case LATTR_WIDE:
	    nfont |= FONT_WIDE;
	    break;
	  default:
	    nfont |= FONT_WIDE + FONT_HIGH;
	    break;
	}
    if (attr & ATTR_NARROW)
	nfont |= FONT_NARROW;

    /* Special hack for the VT100 linedraw glyphs. */
    if (text[0] >= 0x23BA && text[0] <= 0x23BD) {
	switch ((unsigned char) (text[0])) {
	  case 0xBA:
	    text_adjust = -2 * font_height / 5;
	    break;
	  case 0xBB:
	    text_adjust = -1 * font_height / 5;
	    break;
	  case 0xBC:
	    text_adjust = font_height / 5;
	    break;
	  case 0xBD:
	    text_adjust = 2 * font_height / 5;
	    break;
	}
	if (lattr == LATTR_TOP || lattr == LATTR_BOT)
	    text_adjust *= 2;
	text[0] = ucsdata.unitab_xterm['q'];
	if (attr & ATTR_UNDER) {
	    attr &= ~ATTR_UNDER;
	    force_manual_underline = 1;
	}
    }

    /* Anything left as an original character set is unprintable. */
    if (DIRECT_CHAR(text[0])) {
	int i;
	for (i = 0; i < len; i++)
	    text[i] = 0xFFFD;
    }

    /* OEM CP */
    if ((text[0] & CSET_MASK) == CSET_OEMCP)
	nfont |= FONT_OEM;

    nfg = ((attr & ATTR_FGMASK) >> ATTR_FGSHIFT);
    nbg = ((attr & ATTR_BGMASK) >> ATTR_BGSHIFT);
    if (bold_font_mode == BOLD_FONT && (attr & ATTR_BOLD))
	nfont |= FONT_BOLD;
    if (und_mode == UND_FONT && (attr & ATTR_UNDER))
	nfont |= FONT_UNDERLINE;
    another_font( nfont);
    if (!fonts[nfont]) {
	if (nfont & FONT_UNDERLINE)
	    force_manual_underline = 1;
	/* Don't do the same for manual bold, it could be bad news. */

	nfont &= ~(FONT_BOLD | FONT_UNDERLINE);
    }
    another_font( nfont);
    if (!fonts[nfont])
	nfont = FONT_NORMAL;
    if (attr & ATTR_REVERSE) {
	t = nfg;
	nfg = nbg;
	nbg = t;
    }
    if (bold_font_mode == BOLD_NONE && (attr & ATTR_BOLD)) {
	if (nfg < 16) nfg |= 8;
	else if (nfg >= 256) nfg |= 1;
    }
    if (bold_font_mode == BOLD_NONE && (attr & ATTR_BLINK)) {
	if (nbg < 16) nbg |= 8;
	else if (nbg >= 256) nbg |= 1;
    }
    fg = colours[nfg];
    bg = colours[nbg];
    SelectObject(hdc, fonts[nfont]);
    SetTextColor(hdc, fg);
    SetBkColor(hdc, bg);
    if (attr & TATTR_COMBINING)
	SetBkMode(hdc, TRANSPARENT);
    else
	SetBkMode(hdc, OPAQUE);
    line_box.left = x;
    line_box.top = y;
    line_box.right = x + char_width * len;
    line_box.bottom = y + font_height;

    /* Only want the left half of double width lines */
    if (line_box.right > font_width*term->cols+offset_width)
	line_box.right = font_width*term->cols+offset_width;

    if (font_varpitch) {
        /*
         * If we're using a variable-pitch font, we unconditionally
         * draw the glyphs one at a time and centre them in their
         * character cells (which means in particular that we must
         * disable the lpDx mechanism). This gives slightly odd but
         * generally reasonable results.
         */
        xoffset = char_width / 2;
        SetTextAlign(hdc, TA_TOP | TA_CENTER | TA_NOUPDATECP);
        lpDx_maybe = NULL;
        maxlen = 1;
    } else {
        /*
         * In a fixed-pitch font, we draw the whole string in one go
         * in the normal way.
         */
        xoffset = 0;
        SetTextAlign(hdc, TA_TOP | TA_LEFT | TA_NOUPDATECP);
        lpDx_maybe = lpDx;
        maxlen = len;
    }

    opaque = TRUE;                     /* start by erasing the rectangle */
    for (remaining = len; remaining > 0;
         text += len, remaining -= len, x += char_width * len) {
        len = (maxlen < remaining ? maxlen : remaining);

        if (len > lpDx_len) {
            if (len > lpDx_len) {
                lpDx_len = len * 9 / 8 + 16;
                lpDx = sresize(lpDx, lpDx_len, int);
            }
        }
        {
            int i;
            for (i = 0; i < len; i++)
                lpDx[i] = char_width;
        }

        /* We're using a private area for direct to font. (512 chars.) */
        if (ucsdata.dbcs_screenfont && (text[0] & CSET_MASK) == CSET_ACP) {
            /* Ho Hum, dbcs fonts are a PITA! */
            /* To display on W9x I have to convert to UCS */
            static wchar_t *uni_buf = 0;
            static int uni_len = 0;
            int nlen, mptr;
            if (len > uni_len) {
                sfree(uni_buf);
                uni_len = len;
                uni_buf = snewn(uni_len, wchar_t);
            }

            for(nlen = mptr = 0; mptr<len; mptr++) {
                uni_buf[nlen] = 0xFFFD;
                if (IsDBCSLeadByteEx(ucsdata.font_codepage,
                                     (BYTE) text[mptr])) {
                    char dbcstext[2];
                    dbcstext[0] = text[mptr] & 0xFF;
                    dbcstext[1] = text[mptr+1] & 0xFF;
                    lpDx[nlen] += char_width;
                    MultiByteToWideChar(ucsdata.font_codepage, MB_USEGLYPHCHARS,
                                        dbcstext, 2, uni_buf+nlen, 1);
                    mptr++;
                }
                else
                {
                    char dbcstext[1];
                    dbcstext[0] = text[mptr] & 0xFF;
                    MultiByteToWideChar(ucsdata.font_codepage, MB_USEGLYPHCHARS,
                                        dbcstext, 1, uni_buf+nlen, 1);
                }
                nlen++;
            }
            if (nlen <= 0)
                return;		       /* Eeek! */

            ExtTextOutW(hdc, x + xoffset,
                        y - font_height * (lattr == LATTR_BOT) + text_adjust,
                        ETO_CLIPPED | (opaque ? ETO_OPAQUE : 0),
                        &line_box, uni_buf, nlen,
                        lpDx_maybe);
            if (bold_font_mode == BOLD_SHADOW && (attr & ATTR_BOLD)) {
                SetBkMode(hdc, TRANSPARENT);
                ExtTextOutW(hdc, x + xoffset - 1,
                            y - font_height * (lattr ==
                                               LATTR_BOT) + text_adjust,
                            ETO_CLIPPED, &line_box, uni_buf, nlen, lpDx_maybe);
            }

            lpDx[0] = -1;
        } else if (DIRECT_FONT(text[0])) {
            static char *directbuf = NULL;
            static int directlen = 0;
            int i;
            if (len > directlen) {
                directlen = len;
                directbuf = sresize(directbuf, directlen, char);
            }

            for (i = 0; i < len; i++)
                directbuf[i] = text[i] & 0xFF;

            ExtTextOut(hdc, x + xoffset,
                       y - font_height * (lattr == LATTR_BOT) + text_adjust,
                       ETO_CLIPPED | (opaque ? ETO_OPAQUE : 0),
                       &line_box, A2W(directbuf), len, lpDx_maybe);
            if (bold_font_mode == BOLD_SHADOW && (attr & ATTR_BOLD)) {
                SetBkMode(hdc, TRANSPARENT);

                /* GRR: This draws the character outside its box and
                 * can leave 'droppings' even with the clip box! I
                 * suppose I could loop it one character at a time ...
                 * yuk.
                 * 
                 * Or ... I could do a test print with "W", and use +1
                 * or -1 for this shift depending on if the leftmost
                 * column is blank...
                 */
                ExtTextOut(hdc, x + xoffset - 1,
                           y - font_height * (lattr ==
                                              LATTR_BOT) + text_adjust,
                           ETO_CLIPPED, &line_box, A2W(directbuf), len, lpDx_maybe);
            }
        } else {
            /* And 'normal' unicode characters */
            static WCHAR *wbuf = NULL;
            static int wlen = 0;
            int i;

            if (wlen < len) {
                sfree(wbuf);
                wlen = len;
                wbuf = snewn(wlen, WCHAR);
            }

            for (i = 0; i < len; i++)
                wbuf[i] = text[i];

            /* print Glyphs as they are, without Windows' Shaping*/
            general_textout(x + xoffset,
                            y - font_height * (lattr==LATTR_BOT) + text_adjust,
                            &line_box, (short unsigned int*)wbuf, len, lpDx,
                            opaque && !(attr & TATTR_COMBINING));

            /* And the shadow bold hack. */
            if (bold_font_mode == BOLD_SHADOW && (attr & ATTR_BOLD)) {
                SetBkMode(hdc, TRANSPARENT);
                ExtTextOutW(hdc, x + xoffset - 1,
                            y - font_height * (lattr ==
                                               LATTR_BOT) + text_adjust,
                            ETO_CLIPPED, &line_box, wbuf, len, lpDx_maybe);
            }
        }

        /*
         * If we're looping round again, stop erasing the background
         * rectangle.
         */
        SetBkMode(hdc, TRANSPARENT);
        opaque = FALSE;
    }
    if (lattr != LATTR_TOP && (force_manual_underline ||
			       (und_mode == UND_LINE
				&& (attr & ATTR_UNDER)))) {
	HPEN oldpen;
	int dec = descent;
	if (lattr == LATTR_BOT)
	    dec = dec * 2 - font_height;

	oldpen = (HPEN__*)SelectObject(hdc, CreatePen(PS_SOLID, 0, fg));
	MoveToEx(hdc, x, y + dec, NULL);
	LineTo(hdc, x + len * char_width, y + dec);
	oldpen = (HPEN__*)SelectObject(hdc, oldpen);
	DeleteObject(oldpen);
    }
}

/*
 * Wrapper that handles combining characters.
 */
void NativePuttyController::do_text(int x, int y, wchar_t *text, int len,
	     unsigned long attr, int lattr)
{
    if (attr & TATTR_COMBINING) {
	unsigned long a = 0;
	attr &= ~TATTR_COMBINING;
	while (len--) {
	    do_text_internal(x, y, text, 1, attr | a, lattr);
	    text++;
	    a = TATTR_COMBINING;
	}
    } else
	do_text_internal(x, y, text, len, attr, lattr);
}

void NativePuttyController::another_font(int fontno)
{
	USES_CONVERSION;
    int basefont;
    int fw_dontcare, fw_bold;
    int c, u, w, x;
    char *s;

    if (fontno < 0 || fontno >= FONT_MAXNO || fontflag[fontno])
	return;

    basefont = (fontno & ~(FONT_BOLDUND));
    if (basefont != fontno && !fontflag[basefont])
	another_font(basefont);

	FontSpec* font = conf_get_fontspec(cfg, CONF_font);
    if (font->isbold) {
	fw_dontcare = FW_BOLD;
	fw_bold = FW_HEAVY;
    } else {
	fw_dontcare = FW_DONTCARE;
	fw_bold = FW_BOLD;
    }

    c = font->charset;
    w = fw_dontcare;
    u = FALSE;
    s = font->name;
    x = font_width;

    if (fontno & FONT_WIDE)
	x *= 2;
    if (fontno & FONT_NARROW)
	x = (x+1)/2;
    if (fontno & FONT_OEM)
	c = OEM_CHARSET;
    if (fontno & FONT_BOLD)
	w = fw_bold;
    if (fontno & FONT_UNDERLINE)
	u = TRUE;

    fonts[fontno] =
	CreateFont(font_height * (1 + !!(fontno & FONT_HIGH)), x, 0, 0, w,
		   FALSE, u, FALSE, c, OUT_DEFAULT_PRECIS,
		   CLIP_DEFAULT_PRECIS, FONT_QUALITY(conf_get_int(cfg, CONF_font_quality)),
		   DEFAULT_PITCH | FF_DONTCARE, A2W(s));

    fontflag[fontno] = 1;
}

/*
 * The exact_textout() wrapper, unfortunately, destroys the useful
 * Windows `font linking' behaviour: automatic handling of Unicode
 * code points not supported in this font by falling back to a font
 * which does contain them. Therefore, we adopt a multi-layered
 * approach: for any potentially-bidi text, we use exact_textout(),
 * and for everything else we use a simple ExtTextOut as we did
 * before exact_textout() was introduced.
 */
void NativePuttyController::general_textout(int x, int y, CONST RECT *lprc,
			    unsigned short *lpString, UINT cbCount,
			    CONST INT *lpDx, int opaque)
{
    assert(hdc != NULL);
    
    int i, j, xp, xn;
    int bkmode = 0, got_bkmode = FALSE;

    xp = xn = x;

    for (i = 0; i < (int)cbCount ;) {
	int rtl = is_rtl(lpString[i]);

	xn += lpDx[i];

	for (j = i+1; j < (int)cbCount; j++) {
	    if (rtl != is_rtl(lpString[j]))
		break;
	    xn += lpDx[j];
	}

	/*
	 * Now [i,j) indicates a maximal substring of lpString
	 * which should be displayed using the same textout
	 * function.
	 */
	if (rtl) {
	    exact_textout(hdc, xp, y, lprc, lpString+i, j-i,
                          font_varpitch ? NULL : lpDx+i, opaque);
	} else {
	    ExtTextOutW(hdc, xp, y, ETO_CLIPPED | (opaque ? ETO_OPAQUE : 0),
			lprc, (WCHAR*)(lpString+i), j-i,
                        font_varpitch ? NULL : lpDx+i);
	}

	i = j;
	xp = xn;

        bkmode = GetBkMode(hdc);
        got_bkmode = TRUE;
        SetBkMode(hdc, TRANSPARENT);
        opaque = FALSE;
    }

    if (got_bkmode)
        SetBkMode(hdc, bkmode);
}

/*
 * This is a wrapper to ExtTextOut() to force Windows to display
 * the precise glyphs we give it. Otherwise it would do its own
 * bidi and Arabic shaping, and we would end up uncertain which
 * characters it had put where.
 */
void NativePuttyController::exact_textout(HDC hdc, int x, int y, CONST RECT *lprc,
			  unsigned short *lpString, UINT cbCount,
			  CONST INT *lpDx, int opaque)
{
	USES_CONVERSION;
#ifdef __LCC__
    /*
     * The LCC include files apparently don't supply the
     * GCP_RESULTSW type, but we can make do with GCP_RESULTS
     * proper: the differences aren't important to us (the only
     * variable-width string parameter is one we don't use anyway).
     */
    GCP_RESULTS gcpr;
#else
    GCP_RESULTSW gcpr;
#endif
    char *buffer = snewn(cbCount*2+2, char);
    char *classbuffer = snewn(cbCount, char);
    memset(&gcpr, 0, sizeof(gcpr));
    memset(buffer, 0, cbCount*2+2);
    memset(classbuffer, GCPCLASS_NEUTRAL, cbCount);

    gcpr.lStructSize = sizeof(gcpr);
    gcpr.lpGlyphs = (WCHAR*)buffer;
    //gcpr.lpClass = (typeof (gcpr.lpClass))classbuffer;
	gcpr.lpClass = (LPSTR )classbuffer;
    gcpr.nGlyphs = cbCount;
    GetCharacterPlacementW(hdc, (const WCHAR*)lpString, cbCount, 0, &gcpr,
			   FLI_MASK | GCP_CLASSIN | GCP_DIACRITIC);

    ExtTextOut(hdc, x, y,
	       ETO_GLYPH_INDEX | ETO_CLIPPED | (opaque ? ETO_OPAQUE : 0),
	       lprc, A2W(buffer), cbCount, lpDx);
}


void NativePuttyController::real_palette_set(int n, int r, int g, int b)
{
    if (pal) {
    	logpal->palPalEntry[n].peRed = r;
    	logpal->palPalEntry[n].peGreen = g;
    	logpal->palPalEntry[n].peBlue = b;
    	logpal->palPalEntry[n].peFlags = PC_NOCOLLAPSE;
    	colours[n] = PALETTERGB(r, g, b);
    	SetPaletteEntries(pal, 0, NALLCOLOURS, logpal->palPalEntry);
    } else
    	colours[n] = RGB(r, g, b);
}


void NativePuttyController::showPage()
{
	//HWND parent;
	//if (view_->GetWidget() && view_->GetWidget()->GetTopLevelWidget()
	//	&& (parent = view_->GetWidget()->GetTopLevelWidget()->GetNativeView())){
	//	SetParent(page_->getWinHandler(), parent);
	//	ShowWindow(page_->getWinHandler(), SW_SHOW);
	//}
}

void NativePuttyController::hidePage()
{
	//SetParent(page_->getWinHandler(), NULL);
	//ShowWindow(page_->getWinHandler(), SW_HIDE);
}

void NativePuttyController::parentChanged(view::View* parent)
{
	if (parent 
		&& parent->GetWidget() 
		&& parent->GetWidget()->GetTopLevelWidget()
		&& (nativeParentWin_ = parent->GetWidget()->GetTopLevelWidget()->GetNativeView())){
		hTopWnd = nativeParentWin_;
		if (NULL == page_ ){
			init(nativeParentWin_);
		}
		HWND pageHwnd = page_->getWinHandler();
		SetParent(pageHwnd, nativeParentWin_);
		view_->Layout();
		ShowWindow(pageHwnd, SW_SHOW);
		InvalidateRect(nativeParentWin_, NULL, TRUE);
		update_mouse_pointer();
		parent->GetWidget()->GetTopLevelWidget()->Activate();
		assert(IsChild(nativeParentWin_, pageHwnd));
	}else
	{
		ShowWindow(page_->getWinHandler(), SW_HIDE);
		SetParent(page_->getWinHandler(), NULL);
	}
}

#undef IsMinimized
void NativePuttyController::setPagePos(const RECT* rc)
{
	bool getWidget = view_->GetWidget() != NULL;
	bool getTopWindow = getWidget && view_->GetWidget()->GetTopLevelWidget();
	bool isTopWindowNotMinimized = getTopWindow && !view_->GetWidget()->GetTopLevelWidget()->IsMinimized();
	if (/*memcmp(rc, &preRect, sizeof(RECT)) != 0
		&&*/ isTopWindowNotMinimized ){

		preRect = *rc;
		page_->resize( rc, conf_get_int(cfg, CONF_window_border));
		resize_term();
	}
}

void NativePuttyController::resize_term()
{
	if (conf_get_int(cfg, CONF_resize_action) == RESIZE_DISABLED) {
	    /* A resize, well it better be a minimize. */
	    reset_window(RESET_NONE);
	} else {
	    int width, height, w, h;

	    page_->get_term_size(&width, &height);
        prev_rows = term->rows;
        prev_cols = term->cols;
        if (conf_get_int(cfg, CONF_resize_action) == RESIZE_TERM) {
            w = width / font_width;
            if (w < 1) w = 1;
            h = height / font_height;
            if (h < 1) h = 1;
			conf_set_int(cfg, CONF_height, h);
		    conf_set_int(cfg, CONF_width, w);
            term_size(term, h, w, conf_get_int(cfg, CONF_savelines));
			reset_window(RESET_FONT);
        } else if (conf_get_int(cfg, CONF_resize_action) != RESIZE_FONT){
            reset_window(RESET_FONT);
		}else{
            reset_window(RESET_WIN);
		}
	}
	sys_cursor_update();
}

void NativePuttyController::reset_window(int reinit)
{
    /*
     * This function decides how to resize or redraw when the 
     * user changes something. 
     *
     * This function doesn't like to change the terminal size but if the
     * font size is locked that may be it's only soluion.
     */
    int win_width, win_height;
    RECT cr, wr;

    /* Current window sizes ... */
    GetWindowRect(getNativePage(), &wr);
    GetClientRect(getNativePage(), &cr);

    page_->get_term_size(&win_width, &win_height);

    if (conf_get_int(cfg, CONF_resize_action) == RESIZE_DISABLED) reinit = 2;

    /* Are we being forced to reload the fonts ? */
    if (reinit == RESET_FONT) {
    	deinit_fonts();
    	init_fonts(0, font_height_by_wheel);
    }

    /* Oh, looks like we're minimised */
    if (win_width == 0 || win_height == 0)
    	return;

    /* Is the window out of position ? */
    if ( reinit == RESET_WIN && 
	    (offset_width != (win_width-font_width*term->cols)/2 + conf_get_int(cfg, CONF_window_border) ||
	     offset_height != (win_height-font_height*term->rows)/2 + conf_get_int(cfg, CONF_window_border)) ){
	     
        offset_width = (win_width-font_width*term->cols)/2 + conf_get_int(cfg, CONF_window_border);
        offset_height = (win_height-font_height*term->rows)/2 + conf_get_int(cfg, CONF_window_border);
        InvalidateRect(getNativePage(), NULL, TRUE);
    }

    if (conf_get_int(cfg, CONF_resize_action) != RESIZE_TERM) {
    	if (  font_width != win_width/term->cols || 
    		font_height != win_height/term->rows) {
    		  
            deinit_fonts();
            init_fonts(win_width/term->cols, win_height/term->rows);
            offset_width = (win_width-font_width*term->cols)/2 + conf_get_int(cfg, CONF_window_border);
            offset_height = (win_height-font_height*term->rows)/2 + conf_get_int(cfg, CONF_window_border);
            InvalidateRect(getNativePage(), NULL, TRUE);
    	}
    } else {
    	if (  font_width * term->cols != win_width || 
    		font_height * term->rows != win_height) {
    		  
            /* Our only choice at this point is to change the 
                * size of the terminal; Oh well.
                */
            term_size(term, win_height/font_height, win_width/font_width,
                	conf_get_int(cfg, CONF_savelines));
			offset_width = (win_width-font_width*term->cols)/2 + conf_get_int(cfg, CONF_window_border);
			offset_height = (win_height-font_height*term->rows)/2 + conf_get_int(cfg, CONF_window_border);
            InvalidateRect(getNativePage(), NULL, TRUE);
    	}
    }
    return;
}

/*
 * Actually do the job requested by a WM_NETEVENT
 */
void NativePuttyController::enact_pending_netevent()
{
    static int reentering = 0;
    extern int select_result(WPARAM, LPARAM);

    if (reentering)
	return;			       /* don't unpend the pending */

    reentering = 1;
	for (int i = 0; i < pending_param_list.size(); i++)
	{
		ParamPair param_pair = pending_param_list[i];
		select_result(param_pair.wParam, param_pair.lParam);
	}
	pending_param_list.clear();
    reentering = 0;
}


int NativePuttyController::on_net_event(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    /* Notice we can get multiple netevents, FD_READ, FD_WRITE etc
	 * but the only one that's likely to try to overload us is FD_READ.
	 * This means buffering just one is fine.
	 */
	//if (pending_netevent)
	//    enact_pending_netevent();
	base::AutoLock guard(socketTreeLock_);
	ParamPair pair;
	pair.wParam = wParam;
	pair.lParam = lParam;
	pending_param_list.push_back(pair);
	if (is_frozen){ 
		return 0; 
	}
	//if (WSAGETSELECTEVENT(lParam) != FD_READ)
	    enact_pending_netevent();

	if (WSAGETSELECTEVENT(lParam) == FD_CONNECT && isLoading()){
		setConnected();
	}
    return 0;
}

DECL_WINDOWS_FUNCTION(static, BOOL, FlashWindowEx, (PFLASHWINFO));

void init_flashwindow()
{
    HMODULE user32_module = load_system32_dll("user32.dll");
    GET_WINDOWS_FUNCTION(user32_module, FlashWindowEx);
}

BOOL flash_window_ex(HWND hwnd, DWORD dwFlags, UINT uCount, DWORD dwTimeout)
{
    if (p_FlashWindowEx) {
	FLASHWINFO fi;
	fi.cbSize = sizeof(fi);
	fi.hwnd = hwnd;
	fi.dwFlags = dwFlags;
	fi.uCount = uCount;
	fi.dwTimeout = dwTimeout;
	return (*p_FlashWindowEx)(&fi);
    }
    else
	return FALSE; /* shrug */
}

/*
 * Timer for platforms where we must maintain window flashing manually
 * (e.g., Win95).
 */
static void flash_window_timer(void *frontend, unsigned long now)
{
	assert(frontend != NULL);
    NativePuttyController *puttyController = (NativePuttyController *)frontend;
    if (puttyController->flashing && now - puttyController->next_flash >= 0) {
		puttyController->flash_window(1);
    }
}

/*
 * Manage window caption / taskbar flashing, if enabled.
 * 0 = stop, 1 = maintain, 2 = start
 */
void NativePuttyController::flash_window(int mode)
{
    if ((mode == 0) || (conf_get_int(cfg, CONF_beep_ind) == B_IND_DISABLED)) {
	/* stop */
	if (flashing) {
	    flashing = 0;
	    if (p_FlashWindowEx)
		flash_window_ex(getNativePage(), FLASHW_STOP, 0, 0);
	    else
		FlashWindow(getNativePage(), FALSE);
	}

    } else if (mode == 2) {
	/* start */
	if (!flashing) {
	    flashing = 1;
	    if (p_FlashWindowEx) {
		/* For so-called "steady" mode, we use uCount=2, which
		 * seems to be the traditional number of flashes used
		 * by user notifications (e.g., by Explorer).
		 * uCount=0 appears to enable continuous flashing, per
		 * "flashing" mode, although I haven't seen this
		 * documented. */
		flash_window_ex(getNativePage(), FLASHW_ALL | FLASHW_TIMER,
				(conf_get_int(cfg, CONF_beep_ind) == B_IND_FLASH ? 0 : 2),
				0 /* system cursor blink rate */);
		/* No need to schedule timer */
	    } else {
		FlashWindow(getNativePage(), TRUE);
		next_flash = schedule_timer(450, flash_window_timer, this);
	    }
	}

    } else if ((mode == 1) && (conf_get_int(cfg, CONF_beep_ind) == B_IND_FLASH)) {
	/* maintain */
	if (flashing && !p_FlashWindowEx) {
	    FlashWindow(getNativePage(), TRUE);	/* toggle */
	    next_flash = schedule_timer(450, flash_window_timer, this);
	}
    }
}

void NativePuttyController::onSetFocus()
{
	extern HWND hConfigWnd;
	if (hConfigWnd){
		bringToForeground(hConfigWnd);
		return;
	}
	
	//reset the hTopWnd
	extern HWND hTopWnd;
	hTopWnd = WindowInterface::GetInstance()->getNativeTopWnd();

	term_set_focus(term, TRUE);
	::CreateCaret(getNativePage(), caretbm, font_width, font_height);
	ShowCaret(getNativePage());
	flash_window(0);	       /* stop */
	compose_state = 0;
	term_update(term);
}


void NativePuttyController::onKillFocus()
{
	//reset the hTopWnd
	extern HWND hTopWnd;
	hTopWnd = WindowInterface::GetInstance()->getNativeTopWnd();

	show_mouseptr( 1);
	term_set_focus(term, FALSE);
	DestroyCaret();
	caret_x = caret_y = -1;	       /* ensure caret is replaced next time */
	term_update(term);
}
/*
 * Translate a WM_(SYS)?KEY(UP|DOWN) message into a string of ASCII
 * codes. Returns number of bytes used, zero to drop the message,
 * -1 to forward the message to Windows, or another negative number
 * to indicate a NUL-terminated "special" string.
 */
int NativePuttyController::TranslateKey(UINT message, WPARAM wParam, LPARAM lParam,
			unsigned char *output)
{
    BYTE keystate[256];
    int scan, left_alt = 0, key_down, shift_state;
    int r, i, code;
    unsigned char *p = output;
    static int alt_sum = 0;

    HKL kbd_layout = GetKeyboardLayout(0);

    /* keys is for ToAsciiEx. There's some ick here, see below. */
    static WORD keys[3];
    static int compose_char = 0;
    static WPARAM compose_key = 0;

    r = GetKeyboardState(keystate);
    if (!r)
	memset(keystate, 0, sizeof(keystate));
    else {
#if 0
#define SHOW_TOASCII_RESULT
	{			       /* Tell us all about key events */
	    static BYTE oldstate[256];
	    static int first = 1;
	    static int scan;
	    int ch;
	    if (first)
		memcpy(oldstate, keystate, sizeof(oldstate));
	    first = 0;

	    if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) == KF_REPEAT) {
		debug(("+"));
	    } else if ((HIWORD(lParam) & KF_UP)
		       && scan == (HIWORD(lParam) & 0xFF)) {
		debug((". U"));
	    } else {
		debug((".\n"));
		if (wParam >= VK_F1 && wParam <= VK_F20)
		    debug(("K_F%d", wParam + 1 - VK_F1));
		else
		    switch (wParam) {
		      case VK_SHIFT:
			debug(("SHIFT"));
			break;
		      case VK_CONTROL:
			debug(("CTRL"));
			break;
		      case VK_MENU:
			debug(("ALT"));
			break;
		      default:
			debug(("VK_%02x", wParam));
		    }
		if (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP)
		    debug(("*"));
		debug((", S%02x", scan = (HIWORD(lParam) & 0xFF)));

		ch = MapVirtualKeyEx(wParam, 2, kbd_layout);
		if (ch >= ' ' && ch <= '~')
		    debug((", '%c'", ch));
		else if (ch)
		    debug((", $%02x", ch));

		if (keys[0])
		    debug((", KB0=%02x", keys[0]));
		if (keys[1])
		    debug((", KB1=%02x", keys[1]));
		if (keys[2])
		    debug((", KB2=%02x", keys[2]));

		if ((keystate[VK_SHIFT] & 0x80) != 0)
		    debug((", S"));
		if ((keystate[VK_CONTROL] & 0x80) != 0)
		    debug((", C"));
		if ((HIWORD(lParam) & KF_EXTENDED))
		    debug((", E"));
		if ((HIWORD(lParam) & KF_UP))
		    debug((", U"));
	    }

	    if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) == KF_REPEAT);
	    else if ((HIWORD(lParam) & KF_UP))
		oldstate[wParam & 0xFF] ^= 0x80;
	    else
		oldstate[wParam & 0xFF] ^= 0x81;

	    for (ch = 0; ch < 256; ch++)
		if (oldstate[ch] != keystate[ch])
		    debug((", M%02x=%02x", ch, keystate[ch]));

	    memcpy(oldstate, keystate, sizeof(oldstate));
	}
#endif

	if (wParam == VK_MENU && (HIWORD(lParam) & KF_EXTENDED)) {
	    keystate[VK_RMENU] = keystate[VK_MENU];
	}


	/* Nastyness with NUMLock - Shift-NUMLock is left alone though */
	if ((conf_get_int(cfg, CONF_funky_type) == FUNKY_VT400 ||
	     (conf_get_int(cfg, CONF_funky_type) <= FUNKY_LINUX && term->app_keypad_keys &&
	      !conf_get_int(cfg, CONF_no_applic_k)))
	    && wParam == VK_NUMLOCK && !(keystate[VK_SHIFT] & 0x80)) {

	    wParam = VK_EXECUTE;

	    /* UnToggle NUMLock */
	    if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) == 0)
		keystate[VK_NUMLOCK] ^= 1;
	}

	/* And write back the 'adjusted' state */
	SetKeyboardState(keystate);
    }

    /* Disable Auto repeat if required */
    if (term->repeat_off &&
	(HIWORD(lParam) & (KF_UP | KF_REPEAT)) == KF_REPEAT)
	return 0;

    if ((HIWORD(lParam) & KF_ALTDOWN) && (keystate[VK_RMENU] & 0x80) == 0)
	left_alt = 1;

    key_down = ((HIWORD(lParam) & KF_UP) == 0);

    /* Make sure Ctrl-ALT is not the same as AltGr for ToAscii unless told. */
    if (left_alt && (keystate[VK_CONTROL] & 0x80)) {
	if (conf_get_int(cfg, CONF_ctrlaltkeys))
	    keystate[VK_MENU] = 0;
	else {
	    keystate[VK_RMENU] = 0x80;
	    left_alt = 0;
	}
    }

    scan = (HIWORD(lParam) & (KF_UP | KF_EXTENDED | 0xFF));
    shift_state = ((keystate[VK_SHIFT] & 0x80) != 0)
	+ ((keystate[VK_CONTROL] & 0x80) != 0) * 2;

    /* Note if AltGr was pressed and if it was used as a compose key */
    if (!compose_state) {
	compose_key = 0x100;
	if (conf_get_int(cfg, CONF_compose_key)) {
	    if (wParam == VK_MENU && (HIWORD(lParam) & KF_EXTENDED))
		compose_key = wParam;
	}
	if (wParam == VK_APPS)
	    compose_key = wParam;
    }

    if (wParam == compose_key) {
	if (compose_state == 0
	    && (HIWORD(lParam) & (KF_UP | KF_REPEAT)) == 0) compose_state =
		1;
	else if (compose_state == 1 && (HIWORD(lParam) & KF_UP))
	    compose_state = 2;
	else
	    compose_state = 0;
    } else if (compose_state == 1 && wParam != VK_CONTROL)
	compose_state = 0;

    if (compose_state > 1 && left_alt)
	compose_state = 0;

    /* Sanitize the number pad if not using a PC NumPad */
    if (left_alt || (term->app_keypad_keys && !conf_get_int(cfg, CONF_no_applic_k)
		     && conf_get_int(cfg, CONF_funky_type) != FUNKY_XTERM)
	|| conf_get_int(cfg, CONF_funky_type) == FUNKY_VT400 || conf_get_int(cfg, CONF_nethack_keypad) || compose_state) {
	if ((HIWORD(lParam) & KF_EXTENDED) == 0) {
	    int nParam = 0;
	    switch (wParam) {
	      case VK_INSERT:
		nParam = VK_NUMPAD0;
		break;
	      case VK_END:
		nParam = VK_NUMPAD1;
		break;
	      case VK_DOWN:
		nParam = VK_NUMPAD2;
		break;
	      case VK_NEXT:
		nParam = VK_NUMPAD3;
		break;
	      case VK_LEFT:
		nParam = VK_NUMPAD4;
		break;
	      case VK_CLEAR:
		nParam = VK_NUMPAD5;
		break;
	      case VK_RIGHT:
		nParam = VK_NUMPAD6;
		break;
	      case VK_HOME:
		nParam = VK_NUMPAD7;
		break;
	      case VK_UP:
		nParam = VK_NUMPAD8;
		break;
	      case VK_PRIOR:
		nParam = VK_NUMPAD9;
		break;
	      case VK_DELETE:
		nParam = VK_DECIMAL;
		break;
	    }
	    if (nParam) {
		if (keystate[VK_NUMLOCK] & 1)
		    shift_state |= 1;
		wParam = nParam;
	    }
	}
    }

    /* If a key is pressed and AltGr is not active */
    if (key_down && (keystate[VK_RMENU] & 0x80) == 0 && !compose_state) {
	/* Okay, prepare for most alts then ... */
	if (left_alt)
	    *p++ = '\033';

	/* Lets see if it's a pattern we know all about ... */
	if (wParam == VK_PRIOR && shift_state == 1) {
	    SendMessage(getNativePage(), WM_VSCROLL, SB_PAGEUP, 0);
	    return 0;
	}
	if (wParam == VK_PRIOR && shift_state == 2) {
	    SendMessage(getNativePage(), WM_VSCROLL, SB_LINEUP, 0);
	    return 0;
	}
	if (wParam == VK_NEXT && shift_state == 1) {
	    SendMessage(getNativePage(), WM_VSCROLL, SB_PAGEDOWN, 0);
	    return 0;
	}
	if (wParam == VK_NEXT && shift_state == 2) {
	    SendMessage(getNativePage(), WM_VSCROLL, SB_LINEDOWN, 0);
	    return 0;
	}
	if ((wParam == VK_PRIOR || wParam == VK_NEXT) && shift_state == 3) {
	    term_scroll_to_selection(term, (wParam == VK_PRIOR ? 0 : 1));
	    return 0;
	}
	if (wParam == VK_INSERT && shift_state == 1) {
	    request_paste();
	    return 0;
	}
	if (left_alt && wParam == VK_F4 && conf_get_int(cfg, CONF_alt_f4)) {
	    return -1;
	}
	if (left_alt && wParam == VK_SPACE && conf_get_int(cfg, CONF_alt_space)) {
	    SendMessage(getNativePage(), WM_SYSCOMMAND, SC_KEYMENU, 0);
	    return -1;
	}
	if (left_alt && wParam == VK_RETURN && conf_get_int(cfg, CONF_fullscreenonaltenter) &&
	    (conf_get_int(cfg, CONF_resize_action) != RESIZE_DISABLED)) {
 	    if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) != KF_REPEAT)
 		//not support full screen
		//flip_full_screen();
	    return -1;
	}
	/* Control-Numlock for app-keypad mode switch */
	if (wParam == VK_PAUSE && shift_state == 2) {
	    term->app_keypad_keys ^= 1;
	    return 0;
	}

	/* Nethack keypad */
	if (conf_get_int(cfg, CONF_nethack_keypad) && !left_alt) {
	    switch (wParam) {
	      case VK_NUMPAD1:
		*p++ = "bB\002\002"[shift_state & 3];
		return p - output;
	      case VK_NUMPAD2:
		*p++ = "jJ\012\012"[shift_state & 3];
		return p - output;
	      case VK_NUMPAD3:
		*p++ = "nN\016\016"[shift_state & 3];
		return p - output;
	      case VK_NUMPAD4:
		*p++ = "hH\010\010"[shift_state & 3];
		return p - output;
	      case VK_NUMPAD5:
		*p++ = shift_state ? '.' : '.';
		return p - output;
	      case VK_NUMPAD6:
		*p++ = "lL\014\014"[shift_state & 3];
		return p - output;
	      case VK_NUMPAD7:
		*p++ = "yY\031\031"[shift_state & 3];
		return p - output;
	      case VK_NUMPAD8:
		*p++ = "kK\013\013"[shift_state & 3];
		return p - output;
	      case VK_NUMPAD9:
		*p++ = "uU\025\025"[shift_state & 3];
		return p - output;
	    }
	}

	/* Application Keypad */
	if (!left_alt) {
	    int xkey = 0;

	    if (conf_get_int(cfg, CONF_funky_type) == FUNKY_VT400 ||
		(conf_get_int(cfg, CONF_funky_type) <= FUNKY_LINUX &&
		 term->app_keypad_keys && !conf_get_int(cfg, CONF_no_applic_k))) switch (wParam) {
		  case VK_EXECUTE:
		    xkey = 'P';
		    break;
		  case VK_DIVIDE:
		    xkey = 'Q';
		    break;
		  case VK_MULTIPLY:
		    xkey = 'R';
		    break;
		  case VK_SUBTRACT:
		    xkey = 'S';
		    break;
		}
	    if (term->app_keypad_keys && !conf_get_int(cfg, CONF_no_applic_k))
		switch (wParam) {
		  case VK_NUMPAD0:
		    xkey = 'p';
		    break;
		  case VK_NUMPAD1:
		    xkey = 'q';
		    break;
		  case VK_NUMPAD2:
		    xkey = 'r';
		    break;
		  case VK_NUMPAD3:
		    xkey = 's';
		    break;
		  case VK_NUMPAD4:
		    xkey = 't';
		    break;
		  case VK_NUMPAD5:
		    xkey = 'u';
		    break;
		  case VK_NUMPAD6:
		    xkey = 'v';
		    break;
		  case VK_NUMPAD7:
		    xkey = 'w';
		    break;
		  case VK_NUMPAD8:
		    xkey = 'x';
		    break;
		  case VK_NUMPAD9:
		    xkey = 'y';
		    break;

		  case VK_DECIMAL:
		    xkey = 'n';
		    break;
		  case VK_ADD:
		    if (conf_get_int(cfg, CONF_funky_type) == FUNKY_XTERM) {
			if (shift_state)
			    xkey = 'l';
			else
			    xkey = 'k';
		    } else if (shift_state)
			xkey = 'm';
		    else
			xkey = 'l';
		    break;

		  case VK_DIVIDE:
		    if (conf_get_int(cfg, CONF_funky_type) == FUNKY_XTERM)
			xkey = 'o';
		    break;
		  case VK_MULTIPLY:
		    if (conf_get_int(cfg, CONF_funky_type) == FUNKY_XTERM)
			xkey = 'j';
		    break;
		  case VK_SUBTRACT:
		    if (conf_get_int(cfg, CONF_funky_type) == FUNKY_XTERM)
			xkey = 'm';
		    break;

		  case VK_RETURN:
		    if (HIWORD(lParam) & KF_EXTENDED)
			xkey = 'M';
		    break;
		}
	    if (xkey) {
		if (term->vt52_mode) {
		    if (xkey >= 'P' && xkey <= 'S')
			p += sprintf((char *) p, "\x1B%c", xkey);
		    else
			p += sprintf((char *) p, "\x1B?%c", xkey);
		} else
		    p += sprintf((char *) p, "\x1BO%c", xkey);
		return p - output;
	    }
	}

	if (wParam == VK_BACK && shift_state == 0) {	/* Backspace */
	    *p++ = (conf_get_int(cfg, CONF_bksp_is_delete) ? 0x7F : 0x08);
	    *p++ = 0;
	    return -2;
	}
	if (wParam == VK_BACK && shift_state == 1) {	/* Shift Backspace */
	    /* We do the opposite of what is configured */
	    *p++ = (conf_get_int(cfg, CONF_bksp_is_delete) ? 0x08 : 0x7F);
	    *p++ = 0;
	    return -2;
	}
	if (wParam == VK_TAB && shift_state == 1) {	/* Shift tab */
	    *p++ = 0x1B;
	    *p++ = '[';
	    *p++ = 'Z';
	    return p - output;
	}
	if (wParam == VK_SPACE && shift_state == 2) {	/* Ctrl-Space */
	    *p++ = 0;
	    return p - output;
	}
	if (wParam == VK_SPACE && shift_state == 3) {	/* Ctrl-Shift-Space */
	    *p++ = 160;
	    return p - output;
	}
	if (wParam == VK_CANCEL && shift_state == 2) {	/* Ctrl-Break */
	    if (back)
		back->special(backhandle, TS_BRK);
	    return 0;
	}
	if (wParam == VK_PAUSE) {      /* Break/Pause */
	    *p++ = 26;
	    *p++ = 0;
	    return -2;
	}
	/* Control-2 to Control-8 are special */
	if (shift_state == 2 && wParam >= '2' && wParam <= '8') {
	    *p++ = "\000\033\034\035\036\037\177"[wParam - '2'];
	    return p - output;
	}
	if (shift_state == 2 && (wParam == 0xBD || wParam == 0xBF)) {
	    *p++ = 0x1F;
	    return p - output;
	}
	if (shift_state == 2 && (wParam == 0xDF || wParam == 0xDC)) {
	    *p++ = 0x1C;
	    return p - output;
	}
	if (shift_state == 3 && wParam == 0xDE) {
	    *p++ = 0x1E;	       /* Ctrl-~ == Ctrl-^ in xterm at least */
	    return p - output;
	}
	if (shift_state == 0 && wParam == VK_RETURN && term->cr_lf_return) {
	    *p++ = '\r';
	    *p++ = '\n';
	    return p - output;
	}

	/*
	 * Next, all the keys that do tilde codes. (ESC '[' nn '~',
	 * for integer decimal nn.)
	 *
	 * We also deal with the weird ones here. Linux VCs replace F1
	 * to F5 by ESC [ [ A to ESC [ [ E. rxvt doesn't do _that_, but
	 * does replace Home and End (1~ and 4~) by ESC [ H and ESC O w
	 * respectively.
	 */
	code = 0;
	switch (wParam) {
	  case VK_F1:
	    code = (keystate[VK_SHIFT] & 0x80 ? 23 : 11);
	    break;
	  case VK_F2:
	    code = (keystate[VK_SHIFT] & 0x80 ? 24 : 12);
	    break;
	  case VK_F3:
	    code = (keystate[VK_SHIFT] & 0x80 ? 25 : 13);
	    break;
	  case VK_F4:
	    code = (keystate[VK_SHIFT] & 0x80 ? 26 : 14);
	    break;
	  case VK_F5:
	    code = (keystate[VK_SHIFT] & 0x80 ? 28 : 15);
	    break;
	  case VK_F6:
	    code = (keystate[VK_SHIFT] & 0x80 ? 29 : 17);
	    break;
	  case VK_F7:
	    code = (keystate[VK_SHIFT] & 0x80 ? 31 : 18);
	    break;
	  case VK_F8:
	    code = (keystate[VK_SHIFT] & 0x80 ? 32 : 19);
	    break;
	  case VK_F9:
	    code = (keystate[VK_SHIFT] & 0x80 ? 33 : 20);
	    break;
	  case VK_F10:
	    code = (keystate[VK_SHIFT] & 0x80 ? 34 : 21);
	    break;
	  case VK_F11:
	    code = 23;
	    break;
	  case VK_F12:
	    code = 24;
	    break;
	  case VK_F13:
	    code = 25;
	    break;
	  case VK_F14:
	    code = 26;
	    break;
	  case VK_F15:
	    code = 28;
	    break;
	  case VK_F16:
	    code = 29;
	    break;
	  case VK_F17:
	    code = 31;
	    break;
	  case VK_F18:
	    code = 32;
	    break;
	  case VK_F19:
	    code = 33;
	    break;
	  case VK_F20:
	    code = 34;
	    break;
	}
	if ((shift_state&2) == 0) switch (wParam) {
	  case VK_HOME:
	    code = 1;
	    break;
	  case VK_INSERT:
	    code = 2;
	    break;
	  case VK_DELETE:
	    code = 3;
	    break;
	  case VK_END:
	    code = 4;
	    break;
	  case VK_PRIOR:
	    code = 5;
	    break;
	  case VK_NEXT:
	    code = 6;
	    break;
	}
	/* Reorder edit keys to physical order */
	if (conf_get_int(cfg, CONF_funky_type) == FUNKY_VT400 && code <= 6)
	    code = "\0\2\1\4\5\3\6"[code];

	if (term->vt52_mode && code > 0 && code <= 6) {
	    p += sprintf((char *) p, "\x1B%c", " HLMEIG"[code]);
	    return p - output;
	}

	if (conf_get_int(cfg, CONF_funky_type) == FUNKY_SCO &&     /* SCO function keys */
	    code >= 11 && code <= 34) {
	    char codes[] = "MNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@[\\]^_`{";
	    int index = 0;
	    switch (wParam) {
	      case VK_F1: index = 0; break;
	      case VK_F2: index = 1; break;
	      case VK_F3: index = 2; break;
	      case VK_F4: index = 3; break;
	      case VK_F5: index = 4; break;
	      case VK_F6: index = 5; break;
	      case VK_F7: index = 6; break;
	      case VK_F8: index = 7; break;
	      case VK_F9: index = 8; break;
	      case VK_F10: index = 9; break;
	      case VK_F11: index = 10; break;
	      case VK_F12: index = 11; break;
	    }
	    if (keystate[VK_SHIFT] & 0x80) index += 12;
	    if (keystate[VK_CONTROL] & 0x80) index += 24;
	    p += sprintf((char *) p, "\x1B[%c", codes[index]);
	    return p - output;
	}
	if (conf_get_int(cfg, CONF_funky_type) == FUNKY_SCO &&     /* SCO small keypad */
	    code >= 1 && code <= 6) {
	    char codes[] = "HL.FIG";
	    if (code == 3) {
		*p++ = '\x7F';
	    } else {
		p += sprintf((char *) p, "\x1B[%c", codes[code-1]);
	    }
	    return p - output;
	}
	if ((term->vt52_mode || conf_get_int(cfg, CONF_funky_type) == FUNKY_VT100P) && code >= 11 && code <= 24) {
	    int offt = 0;
	    if (code > 15)
		offt++;
	    if (code > 21)
		offt++;
	    if (term->vt52_mode)
		p += sprintf((char *) p, "\x1B%c", code + 'P' - 11 - offt);
	    else
		p +=
		    sprintf((char *) p, "\x1BO%c", code + 'P' - 11 - offt);
	    return p - output;
	}
	if (conf_get_int(cfg, CONF_funky_type) == FUNKY_LINUX && code >= 11 && code <= 15) {
	    p += sprintf((char *) p, "\x1B[[%c", code + 'A' - 11);
	    return p - output;
	}
	if (conf_get_int(cfg, CONF_funky_type) == FUNKY_XTERM && code >= 11 && code <= 14) {
	    if (term->vt52_mode)
		p += sprintf((char *) p, "\x1B%c", code + 'P' - 11);
	    else
		p += sprintf((char *) p, "\x1BO%c", code + 'P' - 11);
	    return p - output;
	}
	if (conf_get_int(cfg, CONF_rxvt_homeend) && (code == 1 || code == 4)) {
	    p += sprintf((char *) p, code == 1 ? "\x1B[H" : "\x1BOw");
	    return p - output;
	}
	if (code) {
	    p += sprintf((char *) p, "\x1B[%d~", code);
	    return p - output;
	}

	/*
	 * Now the remaining keys (arrows and Keypad 5. Keypad 5 for
	 * some reason seems to send VK_CLEAR to Windows...).
	 */
	{
	    char xkey = 0;
	    switch (wParam) {
	      case VK_UP:
		xkey = 'A';
		break;
	      case VK_DOWN:
		xkey = 'B';
		break;
	      case VK_RIGHT:
		xkey = 'C';
		break;
	      case VK_LEFT:
		xkey = 'D';
		break;
	      case VK_CLEAR:
		xkey = 'G';
		break;
	    }
	    if (xkey) {
		p += format_arrow_key((char*)p, term, xkey, shift_state);
		return p - output;
	    }
	}

	/*
	 * Finally, deal with Return ourselves. (Win95 seems to
	 * foul it up when Alt is pressed, for some reason.)
	 */
	if (wParam == VK_RETURN) {     /* Return */
	    *p++ = 0x0D;
	    *p++ = 0;
	    return -2;
	}

	if (left_alt && wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9)
	    alt_sum = alt_sum * 10 + wParam - VK_NUMPAD0;
	else
	    alt_sum = 0;
    }

    /* Okay we've done everything interesting; let windows deal with 
     * the boring stuff */
    {
	BOOL capsOn=0;

	/* helg: clear CAPS LOCK state if caps lock switches to cyrillic */
	if(conf_get_int(cfg, CONF_xlat_capslockcyr) && keystate[VK_CAPITAL] != 0) {
	    capsOn= !left_alt;
	    keystate[VK_CAPITAL] = 0;
	}

	/* XXX how do we know what the max size of the keys array should
	 * be is? There's indication on MS' website of an Inquire/InquireEx
	 * functioning returning a KBINFO structure which tells us. */
	OSVERSIONINFO& osVersion = get_os_version();
	if (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) {
	    /* XXX 'keys' parameter is declared in MSDN documentation as
	     * 'LPWORD lpChar'.
	     * The experience of a French user indicates that on
	     * Win98, WORD[] should be passed in, but on Win2K, it should
	     * be BYTE[]. German WinXP and my Win2K with "US International"
	     * driver corroborate this.
	     * Experimentally I've conditionalised the behaviour on the
	     * Win9x/NT split, but I suspect it's worse than that.
	     * See wishlist item `win-dead-keys' for more horrible detail
	     * and speculations. */
	    BYTE keybs[3];
	    int i;
	    r = ToAsciiEx(wParam, scan, keystate, (LPWORD)keybs, 0, kbd_layout);
	    for (i=0; i<3; i++) keys[i] = keybs[i];
	} else {
	    r = ToAsciiEx(wParam, scan, keystate, keys, 0, kbd_layout);
	}
#ifdef SHOW_TOASCII_RESULT
	if (r == 1 && !key_down) {
	    if (alt_sum) {
		if (in_utf(term) || ucsdata.dbcs_screenfont)
		    debug((", (U+%04x)", alt_sum));
		else
		    debug((", LCH(%d)", alt_sum));
	    } else {
		debug((", ACH(%d)", keys[0]));
	    }
	} else if (r > 0) {
	    int r1;
	    debug((", ASC("));
	    for (r1 = 0; r1 < r; r1++) {
		debug(("%s%d", r1 ? "," : "", keys[r1]));
	    }
	    debug((")"));
	}
#endif
	if (r > 0) {
	    WCHAR keybuf;

	    /*
	     * Interrupt an ongoing paste. I'm not sure this is
	     * sensible, but for the moment it's preferable to
	     * having to faff about buffering things.
	     */
	    term_nopaste(term);

	    p = output;
	    for (i = 0; i < r; i++) {
		unsigned char ch = (unsigned char) keys[i];

		if (compose_state == 2 && (ch & 0x80) == 0 && ch > ' ') {
		    compose_char = ch;
		    compose_state++;
		    continue;
		}
		if (compose_state == 3 && (ch & 0x80) == 0 && ch > ' ') {
		    int nc;
		    compose_state = 0;

		    if ((nc = check_compose(compose_char, ch)) == -1) {
			MessageBeep(MB_ICONHAND);
			return 0;
		    }
		    keybuf = nc;
		    term_seen_key_event(term);
		    if (ldisc)
					luni_send(ldisc, &keybuf, 1, 1);
		    continue;
		}

		compose_state = 0;

		if (!key_down) {
		    if (alt_sum) {
			if (in_utf(term) || ucsdata.dbcs_screenfont) {
			    keybuf = alt_sum;
			    term_seen_key_event(term);
			    if (ldisc)
						luni_send(ldisc, &keybuf, 1, 1);
			} else {
			    ch = (char) alt_sum;
			    /*
			     * We need not bother about stdin
			     * backlogs here, because in GUI PuTTY
			     * we can't do anything about it
			     * anyway; there's no means of asking
			     * Windows to hold off on KEYDOWN
			     * messages. We _have_ to buffer
			     * everything we're sent.
			     */
			    term_seen_key_event(term);
			    if (ldisc){
					if (WindowInterface::GetInstance()->ifNeedCmdScat()){
						WindowInterface::GetInstance()->cmdScat(LDISC_SEND, (char*)&ch, 1, 1);
					}else{
						ldisc_send(ldisc, (char*)&ch, 1, 1);
					}
			    }
			}
			alt_sum = 0;
		    } else {
			term_seen_key_event(term);
			if (ldisc)
					lpage_send(ldisc, kbd_codepage, (char*)&ch, 1, 1);
				}
		} else {
		    if(capsOn && ch < 0x80) {
			WCHAR cbuf[2];
			cbuf[0] = 27;
			cbuf[1] = xlat_uskbd2cyrllic(ch);
			term_seen_key_event(term);
			if (ldisc)
					luni_send(ldisc, cbuf+!left_alt, 1+!!left_alt, 1);
		    } else {
			char cbuf[2];
			cbuf[0] = '\033';
			cbuf[1] = ch;
			term_seen_key_event(term);
			if (ldisc)
			    	lpage_send(ldisc, kbd_codepage,
				       cbuf+!left_alt, 1+!!left_alt, 1);
				}
		    }
		show_mouseptr( 0);
	    }

	    /* This is so the ALT-Numpad and dead keys work correctly. */
	    keys[0] = 0;

	    return p - output;
	}
	/* If we're definitly not building up an ALT-54321 then clear it */
	if (!left_alt)
	    keys[0] = 0;
	/* If we will be using alt_sum fix the 256s */
	else if (keys[0] && (in_utf(term) || ucsdata.dbcs_screenfont))
	    keys[0] = 10;
    }

    /*
     * ALT alone may or may not want to bring up the System menu.
     * If it's not meant to, we return 0 on presses or releases of
     * ALT, to show that we've swallowed the keystroke. Otherwise
     * we return -1, which means Windows will give the keystroke
     * its default handling (i.e. bring up the System menu).
     */
    if (wParam == VK_MENU && !conf_get_int(cfg, CONF_alt_only))
	return 0;

    return -1;
}


int NativePuttyController::on_key(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    /*
	 * Add the scan code and keypress timing to the random
	 * number noise.
	 */
	noise_ultralight(lParam);

	/*
	 * We don't do TranslateMessage since it disassociates the
	 * resulting CHAR message from the KEYDOWN that sparked it,
	 * which we occasionally don't want. Instead, we process
	 * KEYDOWN, and call the Win32 translator functions so that
	 * we get the translations under _our_ control.
	 */
	{
	    unsigned char buf[20];
	    int len;

	    if (wParam == VK_PROCESSKEY) { /* IME PROCESS key */
		if (message == WM_KEYDOWN) {
		    MSG m;
		    m.hwnd = hwnd;
		    m.message = WM_KEYDOWN;
		    m.wParam = wParam;
		    m.lParam = lParam & 0xdfff;
		    TranslateMessage(&m);
		} else return 1; /* pass to Windows for default processing */
	    } else {
		len = TranslateKey(message, wParam, lParam, buf);
		if (len == -1)
		    return DefWindowProc(hwnd, message, wParam, lParam);

		if (len != 0) {
		    /*
		     * Interrupt an ongoing paste. I'm not sure
		     * this is sensible, but for the moment it's
		     * preferable to having to faff about buffering
		     * things.
		     */
		    term_nopaste(term);

		    /*
		     * We need not bother about stdin backlogs
		     * here, because in GUI PuTTY we can't do
		     * anything about it anyway; there's no means
		     * of asking Windows to hold off on KEYDOWN
		     * messages. We _have_ to buffer everything
		     * we're sent.
		     */
		    term_seen_key_event(term);
            if (session_closed && (*buf) == 0x0d){
				if (WindowInterface::GetInstance()->ifNeedCmdScat()){
					WindowInterface::GetInstance()->cmdScat(LDISC_SEND, (char*)&buf, len, 1);
				}
				else{
					SendMessage(hwnd, WM_COMMAND, IDM_RESTART, 0);
				}
                return 0;
            }
		    if (ldisc){
				if (WindowInterface::GetInstance()->ifNeedCmdScat()){
					WindowInterface::GetInstance()->cmdScat(LDISC_SEND, (char*)&buf, len, 1);
				}else{
					ldisc_send(ldisc, (char*)buf, len, 1);
				}
		    }
		    show_mouseptr( 0);
		}
	    }
	}
    return 0;
}

void NativePuttyController::show_mouseptr(int show)
{
    /* NB that the counter in ShowCursor() is also frobbed by
     * update_mouse_pointer() */
    if (!conf_get_int(cfg, CONF_hide_mouseptr))	       /* override if this feature disabled */
    	show = 1;
    if (cursor_visible && !show)
    	ShowCursor(FALSE);
    else if (!cursor_visible && show)
    	ShowCursor(TRUE);
    cursor_visible = show;
}

void NativePuttyController::set_input_locale(HKL kl)
{
    char lbuf[20];

    GetLocaleInfoA(LOWORD(kl), LOCALE_IDEFAULTANSICODEPAGE,
		  lbuf, sizeof(lbuf));

    kbd_codepage = atoi(lbuf);
}


void NativePuttyController::request_paste()
{
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
    CreateThread(NULL, 0, NativePuttyController::clipboard_read_threadfunc,
		 getNativePage(), 0, &in_threadid);
}


DWORD WINAPI NativePuttyController::clipboard_read_threadfunc(void *param)
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

#define X_POS(l) ((int)(short)LOWORD(l))
#define Y_POS(l) ((int)(short)HIWORD(l))

#define TO_CHR_X(x) ((((x)<0 ? (x)-font_width+1 : (x))-offset_width) / font_width)
#define TO_CHR_Y(y) ((((y)<0 ? (y)-font_height+1: (y))-offset_height) / font_height)
 
int NativePuttyController::on_button(HWND hWnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
	if (message == WM_LBUTTONDOWN
		|| message == WM_MBUTTONDOWN
		|| message == WM_RBUTTONDOWN)
    	SetFocus(getNativePage());
    if (message == WM_RBUTTONDOWN &&
    	    ((wParam & MK_CONTROL) || (conf_get_int(cfg, CONF_mouse_is_xterm) == 2))) {
	    POINT cursorpos;

	    show_mouseptr(1);	       /* make sure pointer is visible */
	    GetCursorPos(&cursorpos);
	    TrackPopupMenu(popup_menu,
			   TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
			   cursorpos.x, cursorpos.y,
			   0, hWnd, NULL);
	    return 0;
	}
    {
	    int button, press;

	    switch (message) {
	      case WM_LBUTTONDOWN:
		button = MBT_LEFT;
		wParam |= MK_LBUTTON;
		press = 1;
		break;
	      case WM_MBUTTONDOWN:
		button = MBT_MIDDLE;
		wParam |= MK_MBUTTON;
		press = 1;
		break;
	      case WM_RBUTTONDOWN:
		button = MBT_RIGHT;
		wParam |= MK_RBUTTON;
		press = 1;
		break;
	      case WM_LBUTTONUP:
		button = MBT_LEFT;
		wParam &= ~MK_LBUTTON;
		press = 0;
		break;
	      case WM_MBUTTONUP:
		button = MBT_MIDDLE;
		wParam &= ~MK_MBUTTON;
		press = 0;
		break;
	      case WM_RBUTTONUP:
		button = MBT_RIGHT;
		wParam &= ~MK_RBUTTON;
		press = 0;
		break;
	      default:
		button = press = 0;    /* shouldn't happen */
	    }
	    show_mouseptr(1);
	    /*
	     * Special case: in full-screen mode, if the left
	     * button is clicked in the very top left corner of the
	     * window, we put up the System menu instead of doing
	     * selection.
	     */
	    {
		char mouse_on_hotspot = 0;
		POINT pt;

		GetCursorPos(&pt);
#ifndef NO_MULTIMON
		{
		    HMONITOR mon;
		    MONITORINFO mi;

		    mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);

		    if (mon != NULL) {
			mi.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(mon, &mi);

			if (mi.rcMonitor.left == pt.x &&
			    mi.rcMonitor.top == pt.y) {
			    mouse_on_hotspot = 1;
			}
		    }
		}
#else
		if (pt.x == 0 && pt.y == 0) {
		    mouse_on_hotspot = 1;
		}
#endif
		if (is_full_screen() && press &&
		    button == MBT_LEFT && mouse_on_hotspot) {
		    SendMessage(hWnd, WM_SYSCOMMAND, SC_MOUSEMENU,
				MAKELPARAM(pt.x, pt.y));
		    return 0;
		}
	    }

	    if (press) {
		click((Mouse_Button)button,
		      TO_CHR_X(X_POS(lParam)), TO_CHR_Y(Y_POS(lParam)),
		      wParam & MK_SHIFT, wParam & MK_CONTROL,
		      is_alt_pressed());
		SetCapture(getNativePage());
		isClickingOnPage = true;
	    } else {
		term_mouse(term, (Mouse_Button)button, translate_button((Mouse_Button)button), MA_RELEASE,
			   TO_CHR_X(X_POS(lParam)),
			   TO_CHR_Y(Y_POS(lParam)), wParam & MK_SHIFT,
			   wParam & MK_CONTROL, is_alt_pressed());
		if (!(wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)))
		    ReleaseCapture();
			isClickingOnPage = false;
	    }
	}
    return 0;

}

int NativePuttyController::on_mouse_move(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    {
	    /*
	     * Windows seems to like to occasionally send MOUSEMOVE
	     * events even if the mouse hasn't moved. Don't unhide
	     * the mouse pointer in this case.
	     */
	    static WPARAM wp = 0;
	    static LPARAM lp = 0;
	    if (wParam != wp || lParam != lp ||
		last_mousemove != WM_MOUSEMOVE) {
			show_mouseptr(1);
			wp = wParam; lp = lParam;
			last_mousemove = WM_MOUSEMOVE;
			//InvalidateRect(WindowInterface::GetInstance()->getNativeTopWnd(), NULL, TRUE);
			//InvalidateRect(getNativePage(), NULL, TRUE);
	    }
	}
	/*
	 * Add the mouse position and message time to the random
	 * number noise.
	 */
	noise_ultralight(lParam);

	if (wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)
	     && GetCapture() == getNativePage() && isClickingOnPage) {
	    Mouse_Button b;
	    if (wParam & MK_LBUTTON)
		b = MBT_LEFT;
	    else if (wParam & MK_MBUTTON)
		b = MBT_MIDDLE;
	    else
		b = MBT_RIGHT;
	    term_mouse(term, b, translate_button(b), MA_DRAG,
		       TO_CHR_X(X_POS(lParam)),
		       TO_CHR_Y(Y_POS(lParam)), wParam & MK_SHIFT,
		       wParam & MK_CONTROL, is_alt_pressed());
	}
    return 0;
}

int NativePuttyController::on_nc_mouse_move(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    {
	    static WPARAM wp = 0;
	    static LPARAM lp = 0;
	    if (wParam != wp || lParam != lp ||
		last_mousemove != WM_NCMOUSEMOVE) {
		show_mouseptr(1);
		wp = wParam; lp = lParam;
		last_mousemove = WM_NCMOUSEMOVE;
	    }
	}
	noise_ultralight(lParam);
    return 0;
}

void NativePuttyController::click(Mouse_Button b, int x, int y, int shift, int ctrl, int alt)
{
    int thistime = GetMessageTime();

    if (send_raw_mouse && !(conf_get_int(cfg, CONF_mouse_override) && shift)) {
	lastbtn = MBT_NOTHING;
	term_mouse(term, b, translate_button(b), MA_CLICK,
		   x, y, shift, ctrl, alt);
	return;
    }

    if (lastbtn == b && thistime - lasttime < dbltime) {
	lastact = (lastact == MA_CLICK ? MA_2CLK :
		   lastact == MA_2CLK ? MA_3CLK :
		   lastact == MA_3CLK ? MA_CLICK : MA_NOTHING);
    } else {
	lastbtn = b;
	lastact = MA_CLICK;
    }
    if (lastact != MA_NOTHING)
	term_mouse(term, b, translate_button(b), (Mouse_Action)lastact,
		   x, y, shift, ctrl, alt);
    lasttime = thistime;
}

int NativePuttyController::is_alt_pressed(void)
{
    BYTE keystate[256];
    int r = GetKeyboardState(keystate);
    if (!r)
	return FALSE;
    if (keystate[VK_MENU] & 0x80)
	return TRUE;
    if (keystate[VK_RMENU] & 0x80)
	return TRUE;
    return FALSE;
}

int is_shift_pressed(void)
{
	BYTE keystate[256];
	int r = GetKeyboardState(keystate);
	if (!r)
		return FALSE;
	if (keystate[VK_SHIFT] & 0x80)
		return TRUE;
	//if (keystate[VK_SHIFT] & 0x80)
	//	return TRUE;
	return FALSE;
}

/*
 * Translate a raw mouse button designation (LEFT, MIDDLE, RIGHT)
 * into a cooked one (SELECT, EXTEND, PASTE).
 */
Mouse_Button NativePuttyController::translate_button(Mouse_Button button)
{
    if (button == MBT_LEFT)
		return is_shift_pressed() ? MBT_EXTEND : MBT_SELECT;
    if (button == MBT_MIDDLE)
	return conf_get_int(cfg, CONF_mouse_is_xterm) == 1 ? MBT_PASTE : MBT_EXTEND;
    if (button == MBT_RIGHT)
	return conf_get_int(cfg, CONF_mouse_is_xterm) == 1 ? MBT_EXTEND : MBT_PASTE;
    return (Mouse_Button)0;			       /* shouldn't happen */
}

int NativePuttyController::onMouseWheel(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    if (message == wm_mousewheel || message == WM_MOUSEWHEEL) {
	    int shift_pressed=0, control_pressed=0;

	    if (message == WM_MOUSEWHEEL) {
			wheel_accumulator += (short)HIWORD(wParam);
			shift_pressed=LOWORD(wParam) & MK_SHIFT;
			control_pressed=LOWORD(wParam) & MK_CONTROL;
	    } else {
		BYTE keys[256];
			wheel_accumulator += (int)wParam;
			if (GetKeyboardState(keys)!=0) {
				shift_pressed=keys[VK_SHIFT]&0x80;
				control_pressed=keys[VK_CONTROL]&0x80;
			}
	    }

	    /* process events when the threshold is reached */
	    while (abs(wheel_accumulator) >= WHEEL_DELTA) {
		int b;

		/* reduce amount for next time */
		if (wheel_accumulator > 0) {
		    b = MBT_WHEEL_UP;
		    wheel_accumulator -= WHEEL_DELTA;
		} else if (wheel_accumulator < 0) {
		    b = MBT_WHEEL_DOWN;
		    wheel_accumulator += WHEEL_DELTA;
		} else
		    break;

		if (control_pressed && !shift_pressed) {
			if (font_height_by_wheel == 0) { font_height_by_wheel = font_height; }
			font_height_by_wheel = b == MBT_WHEEL_UP ? font_height_by_wheel + 1 : font_height_by_wheel - 1;
			if (font_height_by_wheel < 6) { font_height_by_wheel = 6; }
			if (font_height_by_wheel > 60) { font_height_by_wheel = 60; }
			char tips[64] = { 0 };
			snprintf(tips, sizeof(tips), "font height:%d", font_height_by_wheel);
			EnableSizeTip(TRUE);
			UpdateSizeStr(page_->getWinHandler(), tips);
			reset_window(RESET_FONT);
			return 1;
		}

		if (send_raw_mouse &&
		    !(conf_get_int(cfg, CONF_mouse_override) && shift_pressed)) {
		    /* Mouse wheel position is in screen coordinates for
		     * some reason */
		    POINT p;
		    p.x = X_POS(lParam); p.y = Y_POS(lParam);
		    if (ScreenToClient(hwnd, &p)) {
			/* send a mouse-down followed by a mouse up */
			term_mouse(term, (Mouse_Button)b, translate_button((Mouse_Button)b),
				   MA_CLICK,
				   TO_CHR_X(p.x),
				   TO_CHR_Y(p.y), shift_pressed,
				   control_pressed, is_alt_pressed());
			term_mouse(term, (Mouse_Button)b, translate_button((Mouse_Button)b),
				   MA_RELEASE, TO_CHR_X(p.x),
				   TO_CHR_Y(p.y), shift_pressed,
				   control_pressed, is_alt_pressed());
		    } /* else: not sure when this can fail */
		} else {
		    /* trigger a scroll */
		    int scrollLines = conf_get_int(cfg, CONF_scrolllines) == -1 ? term->rows/2
		            : conf_get_int(cfg, CONF_scrolllines) == -2          ? term->rows
		            : conf_get_int(cfg, CONF_scrolllines) < -2            ? 3
		            : conf_get_int(cfg, CONF_scrolllines);
		    term_scroll(term, 0,
				b == MBT_WHEEL_UP ?
				-scrollLines : scrollLines);
			if (b == MBT_WHEEL_UP)
			{
				int scroll_to_end = term->scroll_to_end;
				term->scroll_to_end = 0;
				if (scroll_to_end){
					extern void update_toolbar(bool should_restore_state);
					update_toolbar(true);
				}
			}
		}
	    }
	    return 1;
	}

    return 0;
}


void NativePuttyController::setConnected()
{
	backend_state = CONNECTED;
}

void NativePuttyController::setDisconnected()
{
	backend_state = DISCONNECTED;
}

void NativePuttyController::restartBackend()
{
	USES_CONVERSION;
	char *str;
    show_mouseptr(1);
    str = dupprintf("%s Exit Confirmation", disRawName);
    if (!( can_close()||
            MessageBox(getNativePage(),
                    L"Are you sure you want to restart this session?",
					A2W(str), MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON1 | MB_TOPMOST)
            == IDOK)){
        return;
    }
    close_session();
    if (!back) {
		logevent(this, "----- Session is going to be restarted -----");
		const char* str = "\r\n"
			"===============================================================\r\n"
			"--------         trying to restart the session         --------\r\n"
			"===============================================================\r\n";
		term_data(term, 1, str, strlen(str));
	}
	term_pwron(term, FALSE);
	isAutoCmdEnabled_ = TRUE;
	backend_state = LOADING;
	if (start_backend() != 0){
        //MessageBox(WindowInterface::GetInstance()->getNativeTopWnd(), L"failed to start backend!", TEXT("Error"), MB_OK); 
		setDisconnected();
		close_session();
    }
}

int NativePuttyController::on_ime_char(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    if (wParam & 0xFF00) {
	    unsigned char buf[2];

	    buf[1] = wParam;
	    buf[0] = wParam >> 8;
	    term_seen_key_event(term);
	    if (ldisc)
				lpage_send(ldisc, kbd_codepage, (char*)buf, 2, 1);
	} else {
	    char c = (unsigned char) wParam;
	    term_seen_key_event(term);
	    if (ldisc)
				lpage_send(ldisc, kbd_codepage, &c, 1, 1);
			}
    return 0;
}

int NativePuttyController::on_char(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    /*
	 * Nevertheless, we are prepared to deal with WM_CHAR
	 * messages, should they crop up. So if someone wants to
	 * post the things to us as part of a macro manoeuvre,
	 * we're ready to cope.
	 */
	{
	    char c = (unsigned char)wParam;
	    term_seen_key_event(term);
	    if (ldisc)
				lpage_send(ldisc, CP_ACP, &c, 1, 1);
			}
    return 0;
}

int NativePuttyController::on_ime_composition(HWND hwnd, UINT message,
			WPARAM wParam, LPARAM lParam)
{
    HIMC hIMC;
    int n;
    char *buff;
	
	OSVERSIONINFO& osVersion = get_os_version();
    if(osVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS || 
        osVersion.dwPlatformId == VER_PLATFORM_WIN32s) return 1; /* no Unicode */

    if ((lParam & GCS_RESULTSTR) == 0) /* Composition unfinished. */
    	return 1; /* fall back to DefWindowProc */

    hIMC = ImmGetContext(getNativePage());
    n = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);

    if (n > 0) {
	int i;
	buff = snewn(n, char);
	ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, buff, n);
	/*
	 * Jaeyoun Chung reports that Korean character
	 * input doesn't work correctly if we do a single
	 * luni_send() covering the whole of buff. So
	 * instead we luni_send the characters one by one.
	 */
	term_seen_key_event(term);
	for (i = 0; i < n; i += 2) {
	    if (ldisc)
				luni_send(ldisc, (wchar_t* )(buff+i), 1, 1);
			}
	free(buff);
    }
    ImmReleaseContext(getNativePage(), hIMC);
    return 0;
}



int NativePuttyController::on_palette_changed(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    if ((HWND) wParam != hwnd && pal != NULL) {
        NativePuttyController *item = (NativePuttyController*)get_ctx(this);
	    if (item && item->hdc) {
    		if (RealizePalette(item->hdc) > 0)
    		    UpdateColors(item->hdc);
    		free_ctx(item, (Context)item);
	    }
	}
    return 0;
}

int NativePuttyController::on_query_new_palette(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    if (pal != NULL) {
        NativePuttyController *item = (NativePuttyController*)get_ctx(this);
	    if (item && item->hdc) {
		if (RealizePalette(item->hdc) > 0)
		    UpdateColors(item->hdc);
		free_ctx(item, (Context)item);
		return TRUE;
	    }
	}
	return FALSE;
}

void NativePuttyController::send(const char* const buf, const int len)
{
	if (!isDisconnected()) 
		ldisc_send(ldisc, buf, len, 1); 
}

void NativePuttyController::cmdScat(int type, const char * buffer, int buflen, int interactive)
{
	if (isDisconnected()){
		if ((*buffer) == 0x0d){
			restartBackend();
		}
		return;
	}
	if (LDISC_SEND == type)
		ldisc_send(ldisc, buffer, buflen, interactive); 
	else if (LUNI_SEND == type)
		luni_send(ldisc, (wchar_t*)buffer, buflen, interactive);
	else
		lpage_send(ldisc, type, (char*)buffer, buflen, interactive);
}

int NativePuttyController::send_buffer_size()
{
	if (!isDisconnected()) 
		return back->sendbuffer(backhandle);
	return 0;
}


bool NativePuttyController::checkZSession(const char* const recv, const int len)
{
	return zSession_->processNetworkInput(recv, len);
}

void NativePuttyController::closeTab()
{	
	view::PuttyView* puttyView = dynamic_cast<view::PuttyView*>(view_);
	if (puttyView != NULL)
	{
		TabContents* contents = puttyView->GetContents();
		contents->delegate()->CloseContents(contents);
	}
}

void NativePuttyController::rename(const char* input_name)
{
	USES_CONVERSION;
	view::PuttyView* puttyView = dynamic_cast<view::PuttyView*>(view_);
	if (puttyView == NULL){ return; }

	extern const char* show_input_dialog(const char* const caption, const char* tips, char* origin);
	char* origin = W2A(disName.c_str());
	const char* name = input_name;
	if (name == NULL)
	{
		name = show_input_dialog("Change Tab Title", "Please enter the new title", origin);
	}
	if (name == NULL || strlen(name) > 64) { return ; }

	if (strlen(name) == 0){
		char* session_name = conf_get_str(cfg, CONF_session_name);
		char *disrawname = strrchr(session_name, '#');
		disrawname = (disrawname == NULL) ? session_name : (disrawname + 1);
		strncpy(disRawName, disrawname, 256);
		disName = A2W(disrawname);
	}
	else{
		disName = A2W(name);
	}

	TabContents* contents = puttyView->GetContents();
	Browser* browser = dynamic_cast<Browser*>(contents->delegate());
	if (browser == NULL){ return; }
	TabStripModel* tab_strip_model = browser->tabstrip_model();
	int index = tab_strip_model->GetWrapperIndex(contents);
	if (index == TabStripModel::kNoTab){ return; }
	tab_strip_model->UpdateTabContentsStateAt(index, TabStripModelObserver::ALL);
	contents->delegate()->LoadingStateChanged(contents);

	BrowserView* browserView = (BrowserView*)browser->window();
	if (browserView == NULL){ return; }
	TabStrip* tabStrip = (TabStrip*)browserView->tabstrip();
	tabStrip->DoLayout();
	//Browser* browser = BrowserList::GetLastActive();
	//if (browser == NULL){ return; }
	//
	//TabStrip* tabStrip = (TabStrip*)browserView->tabstrip();
	//int activeIndex = browser->tabstrip_model()->active_index();
	//BaseTab* baseTab = tabStrip->base_tab_at_tab_index(activeIndex);
	//TabRendererData data = baseTab->data();
	//data.title = disName;
	//baseTab->SetData(data);
	//browserView->UpdateTitleBar();
}

void NativePuttyController::hide_toolbar()
{
	ToolbarView::is_show_ = !ToolbarView::is_show_;
	save_global_isetting(IF_SHOW_TOOLBAR_SETTING, ToolbarView::is_show_);
	WindowInterface::GetInstance()->AllToolbarSizeChanged(true);
}


bool NativePuttyController::isActive()
{
	return IsWindowVisible(page_->getWinHandler());
}


void NativePuttyController::notifyMsg(const char* msg, void* data)
{
	if (strcmp("show_right_click_menu", msg) == 0){
		POINT cursorpos;
		GetCursorPos(&cursorpos);
		TrackPopupMenu(NativePuttyController::popup_menu,
			TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
			cursorpos.x, cursorpos.y,
			0, getNativePage(), NULL);
	}
}