#include "native_putty_common.h"
#include "native_putty_controller.h"
#include "terminal.h"
#include "storage.h"

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
    return puttyController;
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
