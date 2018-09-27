#include <assert.h>
#include <stdlib.h>

#include "putty.h"
#include "dialog.h"
#include "storage.h"

static void global_key_checkbox_handler(union control *ctrl, void *dlg,
	void *data, int event)
{
	char* key = (char*)ctrl->checkbox.context.p;
	if (event == EVENT_REFRESH) {
		int val = load_global_isetting(key, 1);
		dlg_checkbox_set(ctrl, dlg, val);
	}
	else if (event == EVENT_VALCHANGE) {
		int val = dlg_checkbox_get(ctrl, dlg);
		save_global_isetting(key, val);
	}
}

static void shortcut_type_handler(union control *ctrl, void *dlg,
	void *data, int event)
{
	char* key = (char*)ctrl->editbox.context.p;
	int def_val = ctrl->editbox.context2.i;
	if (event == EVENT_REFRESH) {
		int val = load_global_isetting(key, def_val);
		dlg_update_start(ctrl, dlg);
		dlg_listbox_clear(ctrl, dlg);
		for (int i = 0; i < all_key_type_count; i++){
			dlg_listbox_add(ctrl, dlg, all_shortcut_type_str[i]);
		}
		dlg_editbox_set(ctrl, dlg, all_shortcut_type_str[val]);
		dlg_update_done(ctrl, dlg);
	}
	else if (event == EVENT_VALCHANGE) {
		char *type_str = dlg_editbox_get(ctrl, dlg);
		int val = -1;
		for (int i = 0; i < all_key_type_count; i++){
			if (strcmp(type_str, all_shortcut_type_str[i]) == 0){ val = i; break; }
		}
		if (val > -1){
			save_global_isetting(key, val);
		}
	}
}

static void shortcut_keys_handler(union control *ctrl, void *dlg,
	void *data, int event)
{
	Conf *conf = (Conf *)data;
	if (event == EVENT_REFRESH) {
		int i;
		const char *cp, *thiscp;
		dlg_update_start(ctrl, dlg);
		thiscp = cp_name(decode_codepage(conf_get_str(conf,
			CONF_line_codepage)));
		dlg_listbox_clear(ctrl, dlg);
		for (i = 0; (cp = cp_enumerate(i)) != NULL; i++)
			dlg_listbox_add(ctrl, dlg, cp);
		dlg_editbox_set(ctrl, dlg, thiscp);
		conf_set_str(conf, CONF_line_codepage, thiscp);
		dlg_update_done(ctrl, dlg);
	}
	else if (event == EVENT_VALCHANGE) {
		char *codepage = dlg_editbox_get(ctrl, dlg);
		conf_set_str(conf, CONF_line_codepage,
			cp_name(decode_codepage(codepage)));
		sfree(codepage);
	}
}

void global_setup_config_box(struct controlbox *b)
{
    struct controlset *s;
    union control *c;
	

	ctrl_settitle(b, SHORTCUT_SETTING_NAME, "Global Shortcut Settings");
	s = ctrl_getset(b, SHORTCUT_SETTING_NAME, "general", "Function to Keys(No check for duplicated keys)");
	ctrl_checkbox(s, "Select Tab", '\0', HELPCTX(no_help), global_key_checkbox_handler, P(SHORTCUT_KEY_SELECT_TAB "Enable"));
	ctrl_combobox(s, "", '\0', 20, HELPCTX(no_help), shortcut_type_handler, P(SHORTCUT_KEY_SELECT_TAB "Type"), I(ALT));
//SHORTCUT_KEY_SELECT_TAB "ShortcutKeySelectTab"
//SHORTCUT_KEY_SELECT_NEXT_TAB "ShortcutKeySelectNextTab"
//SHORTCUT_KEY_SELECT_PRE_TAB "ShortcutKeySelectPreTab"
//SHORTCUT_KEY_DUP_TAB "ShortcutKeyDupTab"
//SHORTCUT_KEY_NEW_TAB "ShortcutKeyNewTab"
//SHORTCUT_KEY_RELOAD_TAB "ShortcutKeyReloadTab"
//SHORTCUT_KEY_EDIT_TAB_TITLE "ShortcutKeyEditTabTitle"
//SHORTCUT_KEY_RENAME_SESSION "ShortcutKeyRenameSession"
//SHORTCUT_KEY_HIDE_SHOW_TOOLBAR "ShortcutKeyHideShowToolbar"
//SHORTCUT_KEY_CLOSE_TAB "ShortcutKeyCloseTab"
}
