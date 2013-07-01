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

	return 0;
}

int PuttyTab::fini()
{
	return 0;
}