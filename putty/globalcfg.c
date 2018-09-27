#include <assert.h>
#include <stdlib.h>

#include "putty.h"
#include "dialog.h"
#include "storage.h"

void global_setup_config_box(struct controlbox *b)
{
    struct controlset *s;
    union control *c;
	
	ctrl_settitle(b, SHORTCUT_SETTING_NAME, "Global Shortcut Settings");
    
}
