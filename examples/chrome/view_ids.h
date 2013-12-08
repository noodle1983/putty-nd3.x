
#ifndef __view_ids_h__
#define __view_ids_h__

#pragma once

enum ViewID
{
    VIEW_ID_NONE = 0,

    // BROWSER WINDOW VIEWS
    // ------------------------------------------------------

    // Tabs within a window/tab strip, counting from the left.
    VIEW_ID_TAB_0,
    VIEW_ID_TAB_1,
    VIEW_ID_TAB_2,
    VIEW_ID_TAB_3,
    VIEW_ID_TAB_4,
    VIEW_ID_TAB_5,
    VIEW_ID_TAB_6,
    VIEW_ID_TAB_7,
    VIEW_ID_TAB_8,
    VIEW_ID_TAB_9,
    VIEW_ID_TAB_LAST,

    // ID for any tab. Currently only used on views.
    VIEW_ID_TAB,

    VIEW_ID_TAB_STRIP,

    // Toolbar & toolbar elements.
    VIEW_ID_TOOLBAR = 1000,
    VIEW_ID_NEW_BUTTON,
	VIEW_ID_DUP_BUTTON,
    VIEW_ID_RELOAD_BUTTON,
	VIEW_ID_SESSION_SETTINGS_BUTTON,
	VIEW_ID_COPY_ALL_BUTTON,
	VIEW_ID_CLEAR_BUTTON,
	VIEW_ID_LOG_ENABLER_BUTTON,
	VIEW_ID_SHORTCUT_ENABLER_BUTTON,
	VIEW_ID_CMD_SCATTER_BUTTON,
	VIEW_ID_ABOUT_BUTTON,
	VIEW_ID_FIND_NEXT_BUTTON,
	VIEW_ID_FIND_PREVIOUS_BUTTON,
	VIEW_ID_FIND_RESET_BUTTON,

    VIEW_ID_BACK_BUTTON,
    VIEW_ID_FORWARD_BUTTON,
    VIEW_ID_HOME_BUTTON,
    VIEW_ID_STAR_BUTTON,
    VIEW_ID_LOCATION_BAR,
    VIEW_ID_APP_MENU,
    VIEW_ID_AUTOCOMPLETE,
    VIEW_ID_BROWSER_ACTION_TOOLBAR,
    VIEW_ID_FEEDBACK_BUTTON,

    // The Bookmark Bar.
    VIEW_ID_BOOKMARK_BAR,
    VIEW_ID_OTHER_BOOKMARKS,
    // Used for bookmarks/folders on the bookmark bar.
    VIEW_ID_BOOKMARK_BAR_ELEMENT,

    // Find in page.
    VIEW_ID_FIND_IN_PAGE_TEXT_FIELD,
    VIEW_ID_FIND_IN_PAGE,

    // Tab Container window.
	VIEW_ID_CONTENTS_CONTAINER,
    VIEW_ID_TAB_CONTAINER,
    VIEW_ID_TAB_CONTAINER_FOCUS_VIEW,

    // Docked dev tools.
    VIEW_ID_DEV_TOOLS_DOCKED,

    // The contents split.
    VIEW_ID_CONTENTS_SPLIT,

    // The Infobar container.
    VIEW_ID_INFO_BAR_CONTAINER,

    // The Download shelf.
    VIEW_ID_DOWNLOAD_SHELF,

    // The Sidebar container.
    VIEW_ID_SIDE_BAR_CONTAINER,

    // The sidebar split.
    VIEW_ID_SIDE_BAR_SPLIT,

    // The Compact Navigation bar.
    VIEW_ID_COMPACT_NAV_BAR,

    // The Compact Options bar.
    VIEW_ID_COMPACT_OPT_BAR,

    // The spacer for the compact navigation bar.
    VIEW_ID_COMPACT_NAV_BAR_SPACER,

    // Used in chrome/browser/ui/gtk/view_id_util_browsertests.cc
    // If you add new ids, make sure the above test passes.
    VIEW_ID_PREDEFINED_COUNT
};

#endif //__view_ids_h__