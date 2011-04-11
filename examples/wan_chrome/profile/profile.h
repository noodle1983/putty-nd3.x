
#ifndef __wan_chrome_profile_profile_h__
#define __wan_chrome_profile_profile_h__

#pragma once

#include "base/basic_types.h"

class FilePath;
namespace base
{
    class Time;
}

class BrowserThemeProvider;
class Extension;

typedef intptr_t ProfileId;

class Profile
{
public:
    // Key used to bind profile to the widget with which it is associated.
    static const char* kProfileKey;

    // Value that represents no profile Id.
    static const ProfileId kInvalidProfileId;

    Profile();
    virtual ~Profile() {}

    // Create a new profile given a path.
    static Profile* CreateProfile(const FilePath& path);

    // Returns a unique Id that can be used to identify this profile at runtime.
    // This Id is not persistent and will not survive a restart of the browser.
    virtual ProfileId GetRuntimeId() = 0;

    // Returns the path of the directory where this profile's data is stored.
    virtual FilePath GetPath() = 0;

    // Return the original "recording" profile. This method returns this if the
    // profile is not incognito.
    virtual Profile* GetOriginalProfile() = 0;

    // Init our themes system.
    virtual void InitThemes() = 0;

    // Set the theme to the specified extension.
    virtual void SetTheme(const Extension* extension) = 0;

    // Set the theme to the machine's native theme.
    virtual void SetNativeTheme() = 0;

    // Clear the theme and reset it to default.
    virtual void ClearTheme() = 0;

    // Gets the theme that was last set. Returns NULL if the theme is no longer
    // installed, if there is no installed theme, or the theme was cleared.
    virtual const Extension* GetTheme() = 0;

    // Returns or creates the ThemeProvider associated with this profile
    virtual BrowserThemeProvider* GetThemeProvider() = 0;

    // Return whether 2 profiles are the same. 2 profiles are the same if they
    // represent the same profile. This can happen if there is pointer equality
    // or if one profile is the incognito version of another profile (or vice
    // versa).
    virtual bool IsSameProfile(Profile* profile) = 0;

    // Returns the time the profile was started. This is not the time the profile
    // was created, rather it is the time the user started chrome and logged into
    // this profile. For the single profile case, this corresponds to the time
    // the user started chrome.
    virtual base::Time GetStartTime() const = 0;
};

#endif //__wan_chrome_profile_profile_h__