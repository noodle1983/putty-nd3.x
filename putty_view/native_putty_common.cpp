#include "native_putty_common.h"



#include "putty.h"

void ErrorExit(char * str) 
{ 
    LPVOID lpMsgBuf;
    char* buf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
    if (dw)
        buf = dupprintf("fatal error:%s failed with error (%d: %s)", str, dw, lpMsgBuf);
    else
        buf = dupprintf("fatal error:%s failed", str);

    MessageBox(NULL, (LPCTSTR)buf, TEXT("Error"), MB_OK); 

    sfree(buf);
    LocalFree(lpMsgBuf);
    ExitProcess(dw); 
}

//-----------------------------------------------------------------------

void win_bind_data(HWND hwnd, void *data)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);
}

//-----------------------------------------------------------------------

void* win_get_data(HWND hwnd)
{
    return (void *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

//-----------------------------------------------------------------------

//-----------------------------------------------------------------------

void ltrim(char* str)
{
    int space = strspn(str, " \t");
    memmove(str, str+space, 1+strlen(str)-space);
}

//-----------------------------------------------------------------------
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

//-----------------------------------------------------------------------
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

//-----------------------------------------------------------------------
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

//-----------------------------------------------------------------------

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

