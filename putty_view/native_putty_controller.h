#ifndef NATIVE_PUTTY_CONTROLLER_H
#define NATIVE_PUTTY_CONTROLLER_H

#include "native_putty_common.h"
#include "putty.h"
#include "view/view.h"

class NativePuttyPage;

class NativePuttyController{
public:
	int creat(Config *cfg, int afterIndex);
	int init(Config *cfg, view::View* theView);
	void fini();
	void cfgtopalette();
	void systopalette();
	void init_fonts(const int pick_width, const int pick_height);
	void deinit_fonts();
	int get_font_width(HDC hdc, const TEXTMETRIC *tm);
	int CreateCaret();
	int init_mouse();
	int start_backend();
	void init_palette();
	void close_session();
	int can_close();
	void require_resize(int page_width, int page_height);
	void get_extra_size(int *extra_width, int *extra_height);
	void adjust_text_rect(HDC hdc, const RECT* rc);
	void showPage();
	void hidePage();
	void setPagePos(const RECT* rc);
	void parentChanged(view::View* parent);
	void onSetFocus();

	int on_scroll(HWND hwnd, UINT message,
					WPARAM wParam, LPARAM lParam);
	int on_paint(HWND hwnd, UINT message,
					WPARAM wParam, LPARAM lParam);
	int swallow_shortcut_key(UINT message, WPARAM wParam, LPARAM lParam);
	void update_specials_menu();
	HWND getNativePage();
	void sys_cursor_update();
	void do_text_internal(int x, int y, wchar_t *text, int len,
		      unsigned long attr, int lattr);
	void do_text(int x, int y, wchar_t *text, int len,
	     unsigned long attr, int lattr);
	void another_font(int fontno);
	void general_textout(int x, int y, CONST RECT *lprc,
			    unsigned short *lpString, UINT cbCount,
			    CONST INT *lpDx, int opaque);
	void exact_textout(HDC hdc, int x, int y, CONST RECT *lprc,
			  unsigned short *lpString, UINT cbCount,
			  CONST INT *lpDx, int opaque);
	void real_palette_set(int n, int r, int g, int b);
	void update_mouse_pointer();
	int on_net_event(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	void enact_pending_netevent();
	int on_key(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	int TranslateKey(UINT message, WPARAM wParam, LPARAM lParam,
			unsigned char *output);
	void set_input_locale(HKL kl);
	
	void show_mouseptr(int show);
	void request_paste();
	static DWORD WINAPI clipboard_read_threadfunc(void *param);
int on_menu( HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);

void process_log_status();

public:

    HDC hdc;
    HPALETTE pal;
	
	NativePuttyPage* page_;
	view::View* view_;
    
    Config cfg;
    Terminal *term;
    void *logctx;
    RGBTRIPLE defpal[NALLCOLOURS];
    struct unicode_data ucsdata;

    HFONT fonts[FONT_MAXNO];
    LOGFONT lfont;
    int fontflag[FONT_MAXNO];
    bold_mode_t bold_mode;
    und_mode_t und_mode;
    int extra_width, extra_height;
    int font_width, font_height, font_dualwidth, font_varpitch;
    int offset_width, offset_height;
    int caret_x, caret_y;
    int descent;

    HBITMAP caretbm;

    int dbltime, lasttime, lastact;
    Mouse_Button lastbtn;

    Backend *back;
    void *backhandle;
    void *ldisc;

    int must_close_session, session_closed;

    COLORREF colours[NALLCOLOURS];
    LPLOGPALETTE logpal;

    int send_raw_mouse;
    int wheel_accumulator;
    int busy_status;
    int compose_state;
    UINT wm_mousewheel;

    const struct telnet_special *specials;
    HMENU specials_menu;
    int n_specials;

    int prev_rows, prev_cols;

    int ignore_clip;
  
    HRGN hRgn, hCloserRgn;
	int closerX, closerY;

    RECT rcDis;
    char disRawName[256];
    char disName[256];
    char *window_name, *icon_name;

    HANDLE close_mutex;

    HWND logbox;
    int nevents, negsize;
    char **events;

	int pending_netevent;
	WPARAM pend_netevent_wParam;
	LPARAM pend_netevent_lParam;

	int kbd_codepage;
};

#endif /* NATIVE_PUTTY_CONTROLLER_H */
