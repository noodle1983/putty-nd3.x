#ifndef NATIVE_PUTTY_COMMON_H
#define NATIVE_PUTTY_COMMON_H

#include "putty.h"
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#undef min
#undef max

/* From MSDN: In the WM_SYSCOMMAND message, the four low-order bits of
 * wParam are used by Windows, and should be masked off, so we shouldn't
 * attempt to store information in them. Hence all these identifiers have
 * the low 4 bits clear. Also, identifiers should < 0xF000. */

#define IDM_SHOWLOG   0x0010
#define IDM_NEWSESS   0x0020
#define IDM_DUPSESS   0x0030
#define IDM_RESTART   0x0040
#define IDM_RECONF    0x0050
#define IDM_CLRSB     0x0060
#define IDM_RESET     0x0070
#define IDM_HELP      0x0140
#define IDM_ABOUT     0x0150
#define IDM_SAVEDSESS 0x0160
#define IDM_COPYALL   0x0170
#define IDM_FULLSCREEN	0x0180
#define IDM_PASTE     0x0190
#define IDM_SFTP      0x0200
#define IDM_SPECIALSEP 0x0210
#define IDM_SEARCH_P  0x0220
#define IDM_SEARCH_N  0x0230
#define IDM_SEARCH_R  0x0240
#define IDM_TAB_SHORTCUT  0x0250
#define IDM_START_STOP_LOG  0x0260

#define IDM_SPECIAL_MIN 0x0400
#define IDM_SPECIAL_MAX 0x0800

#define IDM_SAVED_MIN 0x1000
#define IDM_SAVED_MAX 0x5000
#define MENU_SAVED_STEP 16
/* Maximum number of sessions on saved-session submenu */
#define MENU_SAVED_MAX ((IDM_SAVED_MAX-IDM_SAVED_MIN) / MENU_SAVED_STEP)

#define NCFGCOLOURS 22
#define NEXTCOLOURS 240
#define NALLCOLOURS (NCFGCOLOURS + NEXTCOLOURS)

#define FONT_NORMAL 0
#define FONT_BOLD 1
#define FONT_UNDERLINE 2
#define FONT_BOLDUND 3
#define FONT_WIDE	0x04
#define FONT_HIGH	0x08
#define FONT_NARROW	0x10

#define FONT_OEM 	0x20
#define FONT_OEMBOLD 	0x21
#define FONT_OEMUND 	0x22
#define FONT_OEMBOLDUND 0x23

#define FONT_MAXNO 	0x2F
#define FONT_SHIFT	5


#define WM_IGNORE_CLIP (WM_APP + 2)
#define WM_FULLSCR_ON_MAX (WM_APP + 3)
#define WM_AGENT_CALLBACK (WM_APP + 4)
#define WM_GOT_CLIPDATA (WM_APP + 6)
#define WM_CREATE_TAB (WM_APP + 7)
//-----------------------------------------------------------------------
// struct
//-----------------------------------------------------------------------

typedef enum {
    BOLD_NONE, BOLD_SHADOW, BOLD_FONT
} bold_mode_t;
typedef enum {
    UND_LINE, UND_FONT
} und_mode_t;

void ErrorExit(char * str) ;
void win_bind_data(HWND hwnd, void *data);
void* win_get_data(HWND hwnd);
void adjust_host(Conf *cfg);
void bringToForeground(HWND hwnd);

#endif /* NATIVE_PUTTY_COMMON_H */
