#include <assert.h>
#include <stdlib.h>

#include "putty.h"
#include "dialog.h"
#include "storage.h"

void adb_setup_config_box(struct controlbox *b, int midsession,
			  int parity_mask, int flow_mask)
{
    struct controlset *s;
    union control *c;

    if (!midsession) {
		int i;
		extern void config_protocolbuttons_handler(union control *, void *,
							   void *, int);

		/*
		 * Add the serial back end to the protocols list at the
		 * top of the config box.
		 */
		s = ctrl_getset(b, "Session", "hostport",
				"Specify the destination you want to connect to");

		for (i = 0; i < s->ncontrols; i++) {
			c = s->ctrls[i];
			if (c->generic.type == CTRL_RADIO &&
				c->generic.handler == config_protocolbuttons_handler) {
				c->radio.nbuttons++;
				c->radio.ncolumns++;
				c->radio.buttons =
					sresize(c->radio.buttons, c->radio.nbuttons, char *);
				c->radio.buttons[c->radio.nbuttons-1] =
					dupstr("ADB");
				c->radio.buttondata =
					sresize(c->radio.buttondata, c->radio.nbuttons, intorptr);
				c->radio.buttondata[c->radio.nbuttons-1] = I(PROT_ADB);
				if (c->radio.shortcuts) {
					c->radio.shortcuts =
					sresize(c->radio.shortcuts, c->radio.nbuttons, char);
					c->radio.shortcuts[c->radio.nbuttons-1] = 'A';
				}
			}
		}
    }

	ctrl_settitle(b, ANDROID_SETTING_NAME,
		  "ADB Manager Settings");
	s = ctrl_getset(b, ANDROID_SETTING_NAME, "ADB Manager Settings Details",
		"ADB Manager Settings Details");
	ctrl_editbox(s, "Seconds to Scan ADB Device If Needed(0 to turn off)", 's', 20,
		HELPCTX(window_scrollback),
		conf_editbox_handler, I(CONF_adb_dev_scan_interval), I(-1));
    /*
     * Entirely new Connection/Serial panel for serial port
     * configuration.
     */
    ctrl_settitle(b, "Connection/ADB",
		"Options controlling Android Devices"); s = ctrl_getset(b, "Window", "scrollback",
		"Control the scrollback in the window");

    
}
