#ifndef PUTTY_TAB_H
#define PUTTY_TAB_H

#include "putty.h"
#include "SkColor.h"
#include "font.h"
#include "image/image.h"

#include "base/synchronization/lock.h"

#define NCFGCOLOURS 22
#define NEXTCOLOURS 240
#define NALLCOLOURS (NCFGCOLOURS + NEXTCOLOURS)
#define FONT_MAXNO 	0x2F
#define FONT_SHIFT	5

class PuttyTab{
public:
	enum bold_mode_t{
		BOLD_COLOURS, BOLD_SHADOW, BOLD_FONT
	} ;

	enum und_mode_t{
		UND_LINE, UND_FONT
	} ;
private:
    
    Config cfg;
    Terminal *term;
    void *logctx;
    SkColor defpal[NALLCOLOURS];
    struct unicode_data ucsdata;

    gfx::Font fonts[FONT_MAXNO];
    //LOGFONT lfont;
    int fontflag[FONT_MAXNO];
    bold_mode_t bold_mode;
    und_mode_t und_mode;
    int extra_width, extra_height;
    int font_width, font_height, font_dualwidth, font_varpitch;
    int offset_width, offset_height;
    int caret_x, caret_y;
    int descent;

    gfx::Image caretbm;

    int dbltime, lasttime, lastact;
    Mouse_Button lastbtn;

    Backend *back;
    void *backhandle;
    void *ldisc;

    int must_close_session, session_closed;

    SkColor colours[NALLCOLOURS];
    //HPALETTE pal;
    //LPLOGPALETTE logpal;

    int send_raw_mouse;
    int wheel_accumulator;
    int busy_status;
    int compose_state;
    UINT wm_mousewheel;

    const struct telnet_special *specials;
    //HMENU specials_menu;
    //int n_specials;

    int prev_rows, prev_cols;

    int ignore_clip;
  
    char disRawName[256];
    char disName[256];
    char *window_name, *icon_name;

    base::Lock close_mutex;

    //HWND logbox;
    int nevents, negsize;
    char **events;

};
#endif 
