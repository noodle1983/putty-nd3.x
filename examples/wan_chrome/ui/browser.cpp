
#include "browser.h"

#include "base/threading/thread_restrictions.h"
#include "base/utf_string_conversions.h"

#include "../common/notification_service.h"
#include "browser_window.h"

///////////////////////////////////////////////////////////////////////////////
// Browser, Constructors, Creation, Showing:

Browser::Browser(Type type, Profile* profile)
: type_(type),
profile_(profile),
window_(NULL)
{
    registrar_.Add(this, NotificationType::SSL_VISIBLE_STATE_CHANGED,
        NotificationService::AllSources());
    registrar_.Add(this, NotificationType::EXTENSION_UPDATE_DISABLED,
        NotificationService::AllSources());
    registrar_.Add(this, NotificationType::EXTENSION_LOADED,
        NotificationService::AllSources());
    registrar_.Add(this, NotificationType::EXTENSION_UNLOADED,
        NotificationService::AllSources());
    registrar_.Add(this, NotificationType::EXTENSION_UNINSTALLED,
        NotificationService::AllSources());
    registrar_.Add(this, NotificationType::EXTENSION_PROCESS_TERMINATED,
        NotificationService::AllSources());
    registrar_.Add(this, NotificationType::BROWSER_THEME_CHANGED,
        NotificationService::AllSources());
    registrar_.Add(this, NotificationType::PROFILE_ERROR,
        NotificationService::AllSources());

    // Need to know when to alert the user of theme install delay.
    registrar_.Add(this, NotificationType::EXTENSION_READY_FOR_INSTALL,
        NotificationService::AllSources());
}

Browser::~Browser()
{
}

// static
Browser* Browser::Create(Profile* profile)
{
    Browser* browser = new Browser(TYPE_NORMAL, profile);
    browser->InitBrowserWindow();
    return browser;
}

// static
Browser* Browser::CreateForType(Type type, Profile* profile)
{
    Browser* browser = new Browser(type, profile);
    browser->InitBrowserWindow();
    return browser;
}

// static
Browser* Browser::CreateForApp(const std::string& app_name,
                               const gfx::Size& window_size,
                               Profile* profile,
                               bool is_panel)
{
    Browser::Type type = TYPE_APP;

    if(is_panel)
    {
        // TYPE_APP_PANEL is the logical choice.  However, the panel UI
        // is not fully implemented.  See crbug/55943.
        type = TYPE_APP_POPUP;
    }

    Browser* browser = new Browser(type, profile);
    browser->app_name_ = app_name;

    browser->InitBrowserWindow();

    return browser;
}

void Browser::InitBrowserWindow()
{
    DCHECK(!window_);

    window_ = CreateBrowserWindow();

    NotificationService::current()->Notify(
        NotificationType::BROWSER_WINDOW_READY,
        Source<Browser>(this),
        NotificationService::NoDetails());
}

///////////////////////////////////////////////////////////////////////////////
// Browser, protected:

BrowserWindow* Browser::CreateBrowserWindow()
{
    return BrowserWindow::CreateBrowserWindow(this);
}

///////////////////////////////////////////////////////////////////////////////
// Browser, NotificationObserver implementation:

void Browser::Observe(NotificationType type,
                      const NotificationSource& source,
                      const NotificationDetails& details)
{
}