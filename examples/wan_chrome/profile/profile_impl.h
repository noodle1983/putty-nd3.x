
#ifndef __wan_chrome_profile_profile_impl_h__
#define __wan_chrome_profile_profile_impl_h__

#pragma once

#include "base/file_path.h"
#include "base/memory/scoped_ptr.h"

#include "message/timer.h"

#include "../common/notification_observer.h"
#include "../common/notification_registrar.h"

#include "profile.h"

// The default profile implementation.
class ProfileImpl : public Profile, public NotificationObserver
{
public:
    virtual ~ProfileImpl();

    // Profile implementation.
    virtual ProfileId GetRuntimeId();
    virtual FilePath GetPath();
    virtual Profile* GetOriginalProfile();
    virtual void InitThemes();
    virtual void SetTheme(const Extension* extension);
    virtual void SetNativeTheme();
    virtual void ClearTheme();
    virtual const Extension* GetTheme();
    virtual BrowserThemeProvider* GetThemeProvider();
    virtual bool IsSameProfile(Profile* profile);
    virtual base::Time GetStartTime() const;

    // NotificationObserver implementation.
    virtual void Observe(NotificationType type,
        const NotificationSource& source,
        const NotificationDetails& details);

private:
    friend class Profile;

    explicit ProfileImpl(const FilePath& path);

    FilePath GetPrefFilePath();

    NotificationRegistrar registrar_;

    FilePath path_;
    FilePath base_cache_path_;

    scoped_ptr<BrowserThemeProvider> theme_provider_;
    bool created_theme_provider_;

    // See GetStartTime for details.
    base::Time start_time_;

    DISALLOW_COPY_AND_ASSIGN(ProfileImpl);
};

#endif //__wan_chrome_profile_profile_impl_h__