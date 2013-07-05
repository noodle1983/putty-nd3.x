#include "view/view.h"
#include "view/widget/widget.h"


#include "native_putty_controller.h"
#include "native_putty_page.h"
#include "native_putty_common.h"

#include "terminal.h"
#include "storage.h"


#include "atlconv.h" 

int NativePuttyController::init(Config *theCfg, view::View* theView)
{
    hdc = NULL;
    send_raw_mouse = 0;
    wheel_accumulator = 0;
    busy_status = BUSY_NOT;
    compose_state = 0;
    wm_mousewheel = WM_MOUSEWHEEL;
    offset_width = offset_height = cfg.window_border;
    caret_x = -1; 
    caret_y = -1;
    n_specials = 0;
    specials = NULL;
    specials_menu = NULL;
    extra_width = 25;
    extra_height = 28;
    font_width = 10;
    font_height = 20;
    offset_width = offset_height = theCfg->window_border;
    lastact = MA_NOTHING;
    lastbtn = MBT_NOTHING;
    dbltime = GetDoubleClickTime();
    offset_width = theCfg->window_border;
    offset_height = theCfg->window_border;
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
    char *disrawname = strrchr(theCfg->session_name, '#');
    disrawname = (disrawname == NULL)? theCfg->session_name : (disrawname + 1);
    strncpy(disRawName, disrawname, 256);
    close_mutex= CreateMutex(NULL, FALSE, NULL);
    
	page_ = new NativePuttyPage();
	page_->init(&cfg, theView->GetWidget()->GetTopLevelWidget()->GetNativeView());

    win_bind_data(page_->getWinHandler(), this); 
    
    adjust_host(theCfg);
    cfg = *theCfg;
    cfgtopalette();

    
    memset(&ucsdata, 0, sizeof(ucsdata));
    term = term_init(&cfg, &ucsdata, this);
    logctx = log_init(this, &cfg);
    term_provide_logctx(term, logctx);
    term_size(term, cfg.height, 
        cfg.width, cfg.savelines);   
    init_fonts(0, 0);

    CreateCaret();
    page_->init_scrollbar(term);
    init_mouse();
    if (start_backend() != 0){
        MessageBox(NULL, L"failed to start backend!", TEXT("Error"), MB_OK); 
        return -1;
    }

	ShowWindow(page_->getWinHandler(), SW_SHOW);
//    SetForegroundWindow(page.hwndCtrl);

    init_palette();
    term_set_focus(term, TRUE);
    return 0;
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
    if (cfg.protocol == PROT_SSH) {
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
	defpal[w].rgbtRed = cfg.colours[i][0];
	defpal[w].rgbtGreen = cfg.colours[i][1];
	defpal[w].rgbtBlue = cfg.colours[i][2];
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
    if (cfg.system_colour)
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
    TEXTMETRIC tm;
    CPINFO cpinfo;
    int fontsize[3];
    int i;
    HDC hdc;
    int fw_dontcare, fw_bold;
	 USES_CONVERSION; 

    for (i = 0; i < FONT_MAXNO; i++)
    	fonts[i] = NULL;

    bold_mode = cfg.bold_colour ? BOLD_COLOURS : BOLD_FONT;
    und_mode = UND_FONT;

    if (cfg.font.isbold) {
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
    	font_height = cfg.font.height;
    	if (font_height > 0) {
    	    font_height =
    		-MulDiv(font_height, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    	}
    }
    font_width = pick_width;

#define f(i,c,w,u) \
    fonts[i] = CreateFont (font_height, font_width, 0, 0, w, FALSE, u, FALSE, \
			   c, OUT_DEFAULT_PRECIS, \
		           CLIP_DEFAULT_PRECIS, FONT_QUALITY(cfg.font_quality), \
			   FIXED_PITCH | FF_DONTCARE, A2W(cfg.font.name))

    f(FONT_NORMAL, cfg.font.charset, fw_dontcare, FALSE);

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
        font_width = get_font_width( hdc, &tm);
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

    f(FONT_UNDERLINE, cfg.font.charset, fw_dontcare, TRUE);

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
	ExtTextOut(und_dc, 0, 0, ETO_OPAQUE, NULL, L" ", 1, NULL);
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

    if (bold_mode == BOLD_FONT) {
	f(FONT_BOLD, cfg.font.charset, fw_bold, FALSE);
    }
#undef f

    descent = tm.tmAscent + 1;
    if (descent >= font_height)
    	descent = font_height - 1;

    for (i = 0; i < 3; i++) {
	if (fonts[i]) {
	    if (SelectObject(hdc, fonts[i]) && GetTextMetrics(hdc, &tm))
    		fontsize[i] = get_font_width( hdc, &tm) + 256 * tm.tmHeight;
	    else
		fontsize[i] = -i;
	} else
	    fontsize[i] = -i;
    }

	ReleaseDC(page_->hwndCtrl, hdc);

    if (fontsize[FONT_UNDERLINE] != fontsize[FONT_NORMAL]) {
	und_mode = UND_LINE;
	DeleteObject(fonts[FONT_UNDERLINE]);
	fonts[FONT_UNDERLINE] = 0;
    }

    if (bold_mode == BOLD_FONT &&
        	fontsize[FONT_BOLD] != fontsize[FONT_NORMAL]) {
    	bold_mode = BOLD_SHADOW;
    	DeleteObject(fonts[FONT_BOLD]);
    	fonts[FONT_BOLD] = 0;
    }
    fontflag[0] = fontflag[1] = fontflag[2] = 1;
    init_ucs(&cfg, &ucsdata);
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
    char msg[1024] ;
    char *realhost; 
    /*
     * Select protocol. This is farmed out into a table in a
     * separate file to enable an ssh-free variant.
     */
    back = backend_from_proto(cfg.protocol);
    if (back == NULL) {
    	char *str = dupprintf("%s Internal Error", appname);
    	MessageBox(NULL, L"Unsupported protocol number found",
    		   A2W(str), MB_OK | MB_ICONEXCLAMATION);
    	sfree(str);
	    return -1;
    }

    error = back->init(this, &backhandle, &cfg,
		       cfg.host, cfg.port, &realhost, cfg.tcp_nodelay,
		       cfg.tcp_keepalives);
    back->provide_logctx(backhandle, logctx);
    if (error) {
    	char *str = dupprintf("%s Error", appname);
    	sprintf(msg, "Unable to open connection to\n"
    		"%.800s\n" "%s", cfg_dest(&cfg), error);
    	MessageBox(NULL, A2W(msg), A2W(str), MB_ICONERROR | MB_OK);
    	sfree(str);
	    return -1;
    }

    sfree(realhost);

    /*
     * Connect the terminal to the backend for resize purposes.
     */
    term_provide_resize_fn(term, back->size, backhandle);

    /*
     * Set up a line discipline.
     */
    ldisc = ldisc_create(&cfg, term
                    , back, backhandle, this);

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
	if (cfg.try_palette && GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) {
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

    SetWindowPos(page.hwndCtrl, NULL, 0, 0, 
        page_width, page_height, SWP_NOMOVE | SWP_NOZORDER); 
    //MoveWindow(page.hwndCtrl, rc->left, rc->top, 
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
    if (cfg.warn_on_close && !session_closed)
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

int NativePuttyController::on_paint(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
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
/*
int NativePuttyController::swallow_shortcut_key(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message != WM_KEYDOWN && message != WM_SYSKEYDOWN)
        return 0;

    wintab* tab = (wintab*)parentTab;
    int btn_state = SendMessage(tab->hToolBar, 
            TB_GETSTATE , (WPARAM)IDM_TAB_SHORTCUT, (LPARAM)0);
    if (btn_state == -1 || !(btn_state & TBSTATE_CHECKED))
        return 0;
        
    BYTE keystate[256];
    if (GetKeyboardState(keystate) == 0)
        return 0;

    int ctrl_pressed = (keystate[VK_CONTROL] & 0x80);
    int shift_pressed = (keystate[VK_SHIFT] & 0x80);
    int alt_pressed = (keystate[VK_MENU] & 0x80);
    int next_tab = -1;

    if (alt_pressed && !ctrl_pressed && !shift_pressed){
        if (wParam == '0'){
            next_tab = 9;
        }else if (wParam >= '1' && wParam <= '9'){
            next_tab = wParam - '1';
        }else if (wParam == VK_OEM_3 || wParam == VK_RIGHT){
            // '`'
            next_tab = (tab->cur + 1) % tab->end;
        }else if (wParam == VK_LEFT){
            next_tab = (tab->cur + tab->end - 1) % tab->end;
        }

        if (next_tab >= 0 && next_tab < tab->end){
            tab->next = next_tab;
            wintab_swith_tab(tab);
            return 1;
        }
    }

	if (!alt_pressed && ctrl_pressed && !shift_pressed){
		if (wParam == VK_TAB){
			tab->next = (tab->cur + 1) % tab->end;
			wintab_swith_tab(tab);
            return 1;
		}
		else if (wParam == VK_OEM_3){
			wintab_move_tab(tab, 1);;
            return 1;
		}
		else if (wParam == VK_RIGHT){
            wintab_move_tab(tab, 1);
			return 1;
        }else if (wParam == VK_LEFT){
            wintab_move_tab(tab, 0);
			return 1;
        }
	}
	if (!alt_pressed && ctrl_pressed && shift_pressed){
		if (wParam == VK_TAB){
			tab->next = (tab->cur + tab->end - 1) % tab->end;
			wintab_swith_tab(tab);
            return 1;
		}
		else if (wParam == VK_OEM_3){
			wintab_move_tab(tab, 0);
            return 1;
		}
	}
    if (!alt_pressed && ctrl_pressed && shift_pressed){
        if (wParam == 'T'){
            wintab_dup_tab(tab, &cfg);
            return 1;
        }
		if (wParam == 'N'){
            on_session_menu(hwnd, 0,IDM_NEWSESS, 0);
            return 1;
        }
    }
    return 0;

}

*/