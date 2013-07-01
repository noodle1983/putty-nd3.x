#include "putty_tab.h"
//-----------------------------------------------------------------------

void ltrim(char* str)
{
    int space = strspn(str, " \t");
    memmove(str, str+space, 1+strlen(str)-space);
}

/* 
 * See if host is of the form user@host
 */
void takeout_username_from_host(Config *cfg)
{
	if (cfg->host[0] != '\0') {
	    char *atsign = strrchr(cfg->host, '@');
	    /* Make sure we're not overflowing the user field */
	    if (atsign) {
    		if (atsign - cfg->host < sizeof cfg->username) {
    		    strncpy(cfg->username, cfg->host, atsign - cfg->host);
    		    cfg->username[atsign - cfg->host] = '\0';
    		}
    		memmove(cfg->host, atsign + 1, 1 + strlen(atsign + 1));
	    }
	}
}

/*
 * Trim a colon suffix off the hostname if it's there. In
 * order to protect IPv6 address literals against this
 * treatment, we do not do this if there's _more_ than one
 * colon.
 */
void handle_host_colon(char *host)
{
    char *c = strchr(host, ':');

    if (c) {
	char *d = strchr(c+1, ':');
	if (!d)
	    *c = '\0';
    }

}


/*
 * Remove any remaining whitespace from the hostname.
 */
void rm_whitespace(char *host)
{
    int p1 = 0, p2 = 0;
    while (host[p2] != '\0') {
    	if (host[p2] != ' ' && host[p2] != '\t') {
    	    host[p1] = host[p2];
    	    p1++;
    	}
    	p2++;
    }
    host[p1] = '\0';
}

void adjust_host(Config *cfg)
{
    /*
	 * Trim leading whitespace off the hostname if it's there.
	 */
    ltrim(cfg->host);
	
	/* See if host is of the form user@host */
	takeout_username_from_host(cfg);

	/*
	 * Trim a colon suffix off the hostname if it's there. In
	 * order to protect IPv6 address literals against this
	 * treatment, we do not do this if there's _more_ than one
	 * colon.
	 */
	handle_host_colon(cfg->host);

	/*
	 * Remove any remaining whitespace from the hostname.
	 */
	rm_whitespace(cfg->host);
}

//-----------------------------------------------------------------------
//class PuttyTab
PuttyTab::PuttyTab()
{

}

PuttyTab::~PuttyTab()
{
}

int PuttyTab::init(Config& theCfg)
{
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
    extra_width = 25;
    extra_height = 28;
    font_width = 10;
    font_height = 20;
    offset_width = offset_height = theCfg.window_border;
    lastact = MA_NOTHING;
    lastbtn = MBT_NOTHING;
    dbltime = GetDoubleClickTime();
    offset_width = theCfg.window_border;
    offset_height = theCfg.window_border;
    ignore_clip = FALSE;
    nevents = 0;
    negsize = 0;
    events = NULL;
    ldisc = NULL;  

    char *disrawname = strrchr(theCfg.session_name, '#');
    disrawname = (disrawname == NULL)? theCfg.session_name : (disrawname + 1);

	adjust_host(&cfg);
	cfgtopalette();

	memset(&ucsdata, 0, sizeof(ucsdata));
	term = term_init(&cfg, &ucsdata, this);
    logctx = log_init(this, &cfg);
    term_provide_logctx(term, logctx);
    term_size(term, cfg.height, cfg.width, cfg.savelines); 


	return 0;
}

int PuttyTab::CreateCaret()
{
    /*
     * Set up a caret bitmap, with no content.
     */
	//caretbm = CreateBitmap(tabitem->font_width, tabitem->font_height, 1, 1, bits);
	caretbm.setConfig(SkBitmap::kA1_Config, font_width, font_height);
	caretbm.eraseARGB(0, 0, 0, 0);

    CreateCaret(tabitem->page.hwndCtrl, tabitem->caretbm, 
        tabitem->font_width, tabitem->font_height);
    return 0;
}

/*
 * Copy the colour palette from the configuration data into defpal.
 * This is non-trivial because the colour indices are different.
 */
void PuttyTab::cfgtopalette()
{
    int i;
    static const int ww[] = {
	256, 257, 258, 259, 260, 261,
	0, 8, 1, 9, 2, 10, 3, 11,
	4, 12, 5, 13, 6, 14, 7, 15
    };

    for (i = 0; i < 22; i++) {
		int w = ww[i];
		defpal[w] = SkColorSetRGB(cfg.colours[i][0], cfg.colours[i][1], cfg.colours[i][2]);
    }
    for (i = 0; i < NEXTCOLOURS; i++) {
		if (i < 216) {
			int r = i / 36, g = (i / 6) % 6, b = i % 6;
			defpal[i+16] = SkColorSetRGB(
				(r ? r * 40 + 55 : 0), 
				(g ? g * 40 + 55 : 0), 
				(b ? b * 40 + 55 : 0));
		} else {
			int shade = i - 216;
			shade = shade * 10 + 8;
			defpal[i+16] = SkColorSetRGB(shade, shade, shade);
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
void PuttyTab::systopalette()
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
		defpal[OR[i].norm] = SkColorSetRGB(GetRValue(colour), GetGValue(colour), GetBValue(colour));
		defpal[OR[i].bold] = SkColorSetRGB(GetRValue(colour), GetGValue(colour), GetBValue(colour));
    }
}


int PuttyTab::fini()
{
	return 0;
}