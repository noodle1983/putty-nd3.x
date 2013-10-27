#ifndef NATIVE_PUTTY_CONTROLLER_H
#define NATIVE_PUTTY_CONTROLLER_H

#include "base/synchronization/lock.h"
#include "native_putty_common.h"
#include "putty.h"
#include "view/view.h"
#include "base/timer.h"
#include "zmodem_session.h"

class NativePuttyPage;

class NativePuttyController{
public:
	NativePuttyController(Config *cfg, view::View* theView);
	~NativePuttyController();
	int creat(Config *cfg, int afterIndex);
	int init(HWND hwndParent);
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
	void onKillFocus();
	void send(const char* const buf, const int len);

	int on_scroll(HWND hwnd, UINT message,
					WPARAM wParam, LPARAM lParam);
	int on_paint();
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
	int on_button(HWND hWnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	int on_mouse_move(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	int on_nc_mouse_move(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	int onMouseWheel(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	int on_char(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	int on_ime_char(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	int on_ime_composition(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	int on_palette_changed(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	int on_query_new_palette(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	int is_full_screen(){return FALSE;}
	void click(Mouse_Button b, int x, int y, int shift, int ctrl, int alt);
	static int is_alt_pressed(void);
	Mouse_Button translate_button(Mouse_Button button);

	void flash_window(int mode);

	void resize_term();
	void reset_window(int reinit);
	enum{RESET_NONE = -1, RESET_WIN = 0, RESET_FONT = 2};

	void setConnected();
	void setDisconnected();
	bool isLoading(){return backend_state == LOADING;}
	bool isConnected(){return backend_state == CONNECTED;}
	bool isDisconnected(){return backend_state == DISCONNECTED;}
	void restartBackend();
	void checkTimerCallback();

	int on_menu( HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam);
	int on_reconfig();
	void process_log_status();
	bool checkZSession(const char* const recv, const int len, std::string& output)
	{return zSession_.processNetworkInput(recv, len, output);}

public:

    HDC hdc;
    HPALETTE pal;
	
	NativePuttyPage* page_;
	view::View* view_;
    
    Config* cfg;
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
    static UINT wm_mousewheel;

    const struct telnet_special *specials;
    HMENU specials_menu;
    int n_specials;

    int prev_rows, prev_cols;

    int ignore_clip;
  
    HRGN hRgn, hCloserRgn;
	int closerX, closerY;

    RECT rcDis;
    char disRawName[256];
    string16 disName;
    char *window_name, *icon_name;

    HANDLE close_mutex;

    HWND logbox;
    unsigned nevents;
	unsigned negsize;
    char **events;

	int pending_netevent;
	WPARAM pend_netevent_wParam;
	LPARAM pend_netevent_lParam;

	static int kbd_codepage;
	UINT last_mousemove;
	bool isClickingOnPage;

	enum BackendState{LOADING = 0, CONNECTED = 1, DISCONNECTED = -1};
	BackendState backend_state;

	static HMENU popup_menu;

	long next_flash;
	int flashing;

	
    int cursor_visible;
    int forced_visible;

	enum{TIMER_INTERVAL = 50}; //in ms
	base::RepeatingTimer<NativePuttyController> checkTimer_;
	static base::Lock socketTreeLock_;

	ZmodemSession zSession_;

};
#endif /* NATIVE_PUTTY_CONTROLLER_H */
