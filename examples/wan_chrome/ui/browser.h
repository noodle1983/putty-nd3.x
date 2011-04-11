
#ifndef __wan_chrome_ui_browser_h__
#define __wan_chrome_ui_browser_h__

#pragma once

#include <vector>

#include "base/string16.h"

#include "gfx/rect.h"

#include "../common/notification_observer.h"
#include "../common/notification_registrar.h"

class BrowserWindow;
class Profile;

class Browser : public NotificationObserver
{
public:
    // SessionService::WindowType mirrors these values.  If you add to this
    // enum, look at SessionService::WindowType to see if it needs to be
    // updated.
    enum Type
    {
        TYPE_NORMAL = 1,
        TYPE_POPUP = 2,
        // The old-style app created via "Create application shortcuts".
        // Shortcuts to a URL and shortcuts to an installed application
        // both have this type.
        TYPE_APP = 4,
        TYPE_APP_POPUP = TYPE_APP | TYPE_POPUP,
        TYPE_DEVTOOLS = TYPE_APP | 8,

        // TODO(skerner): crbug/56776: Until the panel UI is complete on all
        // platforms, apps that set app.launch.container = "panel" have type
        // APP_POPUP. (see Browser::CreateForApp)
        // NOTE: TYPE_APP_PANEL is a superset of TYPE_APP_POPUP.
        TYPE_APP_PANEL = TYPE_APP | TYPE_POPUP | 16,
        TYPE_ANY = TYPE_NORMAL |
        TYPE_POPUP |
        TYPE_APP |
        TYPE_DEVTOOLS |
        TYPE_APP_PANEL
    };

    // Possible elements of the Browser window.
    enum WindowFeature
    {
        FEATURE_NONE = 0,
        FEATURE_TITLEBAR = 1,
        FEATURE_TABSTRIP = 2,
        FEATURE_TOOLBAR = 4,
        FEATURE_LOCATIONBAR = 8,
        FEATURE_BOOKMARKBAR = 16,
        FEATURE_INFOBAR = 32,
        FEATURE_SIDEBAR = 64,
        FEATURE_DOWNLOADSHELF = 128
    };

    // Constructors, Creation, Showing //////////////////////////////////////////

    // Creates a new browser of the given |type| and for the given |profile|. The
    // Browser has a NULL window after its construction, InitBrowserWindow must
    // be called after configuration for window() to be valid.
    // Avoid using this constructor directly if you can use one of the Create*()
    // methods below. This applies to almost all non-testing code.
    Browser(Type type, Profile* profile);
    virtual ~Browser();

    // Creates a normal tabbed browser with the specified profile. The Browser's
    // window is created by this function call.
    static Browser* Create(Profile* profile);

    // Like Create, but creates a browser of the specified type.
    static Browser* CreateForType(Type type, Profile* profile);

    // Like Create, but creates a toolbar-less "app" window for the specified
    // app. |app_name| is required and is used to identify the window to the
    // shell.  If |extension| is set, it is used to determine the size of the
    // window to open.
    static Browser* CreateForApp(const std::string& app_name,
        const gfx::Size& window_size,
        Profile* profile,
        bool is_panel);

    // Creates the Browser Window. Prefer to use the static helpers above where
    // possible. This does not show the window. You need to call window()->Show()
    // to show it.
    void InitBrowserWindow();

    // Accessors ////////////////////////////////////////////////////////////////

    Type type() const { return type_; }
    const std::string& app_name() const { return app_name_; }
    Profile* profile() const { return profile_; }

    // |window()| will return NULL if called before |CreateBrowserWindow()|
    // is done.
    BrowserWindow* window() const { return window_; }

protected:
    // Wrapper for the factory method in BrowserWindow. This allows subclasses to
    // set their own window.
    virtual BrowserWindow* CreateBrowserWindow();

private:
    // Overridden from NotificationObserver:
    virtual void Observe(NotificationType type,
        const NotificationSource& source,
        const NotificationDetails& details);

    // Data members /////////////////////////////////////////////////////////////

    NotificationRegistrar registrar_;

    // This Browser's type.
    const Type type_;

    // This Browser's profile.
    Profile* const profile_;

    // This Browser's window.
    BrowserWindow* window_;

    // An optional application name which is used to retrieve and save window
    // positions.
    std::string app_name_;

    DISALLOW_COPY_AND_ASSIGN(Browser);
};

#endif //__wan_chrome_ui_browser_h__