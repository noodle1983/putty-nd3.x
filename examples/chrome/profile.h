
#ifndef __profile_h__
#define __profile_h__

#include <string>

#include "base/logging.h"

namespace base
{
    class Time;
}

class AutocompleteClassifier;
class BookmarkModel;
class FaviconService;
class FilePath;
class FindBarState;
class PrefService;

typedef int LocalProfileId;

class Profile
{
public:
    class Delegate
    {
    public:
        // Called when creation of the profile is finished.
        virtual void OnProfileCreated(Profile* profile, bool success) = 0;
    };

    // Key used to bind profile to the widget with which it is associated.
    static const char* const kProfileKey;

    // Value that represents no local profile id.
    static const LocalProfileId kInvalidLocalProfileId;

    Profile();
    virtual ~Profile() {}

    // Profile prefs are registered as soon as the prefs are loaded for the first
    // time.
    static void RegisterUserPrefs(PrefService* prefs);

    // Create a new profile given a path.
    static Profile* CreateProfile(const FilePath& path);

    // Same as above, but uses async initialization.
    static Profile* CreateProfileAsync(const FilePath& path,
        Delegate* delegate);

    // content::BrowserContext implementation ------------------------------------

    virtual FilePath GetPath() = 0;

    // content::BrowserContext implementation ------------------------------------

    // Returns the name associated with this profile. This name is displayed in
    // the browser frame.
    virtual std::string GetProfileName() = 0;

    // Return the original "recording" profile. This method returns this if the
    // profile is not incognito.
    virtual Profile* GetOriginalProfile() = 0;

    // Retrieves a pointer to the FaviconService associated with this
    // profile.  The FaviconService is lazily created the first time
    // that this method is called.
    //
    // Although FaviconService is refcounted, this will not addref, and callers
    // do not need to do any reference counting as long as they keep the pointer
    // only for the local scope (which they should do anyway since the browser
    // process may decide to shut down).
    //
    // |access| defines what the caller plans to do with the service. See
    // the ServiceAccessType definition above.
    virtual FaviconService* GetFaviconService() = 0;

    // Retrieves a pointer to the AutocompleteClassifier associated with this
    // profile. The AutocompleteClassifier is lazily created the first time that
    // this method is called.
    virtual AutocompleteClassifier* GetAutocompleteClassifier() = 0;

    // Retrieves a pointer to the PrefService that manages the preferences
    // for this user profile.  The PrefService is lazily created the first
    // time that this method is called.
    virtual PrefService* GetPrefs() = 0;

    // Returns the find bar state for this profile.  The find bar state is lazily
    // created the first time that this method is called.
    virtual FindBarState* GetFindBarState() = 0;

    // Returns true if the last time this profile was open it was exited cleanly.
    virtual bool DidLastSessionExitCleanly() = 0;

    // Returns the BookmarkModel, creating if not yet created.
    virtual BookmarkModel* GetBookmarkModel() = 0;

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

    // Marks the profile as cleanly shutdown.
    //
    // NOTE: this is invoked internally on a normal shutdown, but is public so
    // that it can be invoked when the user logs out/powers down (WM_ENDSESSION).
    virtual void MarkAsCleanShutdown() = 0;

    // Start up service that gathers data from a promo resource feed.
    virtual void InitPromoResources() = 0;

    // Returns the last directory that was chosen for uploading or opening a file.
    virtual FilePath last_selected_directory() = 0;
    virtual void set_last_selected_directory(const FilePath& path) = 0;

    std::string GetDebugName();

    // Returns whether it is a guest session.
    static bool IsGuestSession();

    // Did the user restore the last session? This is set by SessionRestore.
    void set_restored_last_session(bool restored_last_session)
    {
        restored_last_session_ = restored_last_session;
    }
    bool restored_last_session() const
    {
        return restored_last_session_;
    }

    bool first_launched() const
    {
        return first_launched_;
    }

    // Stop sending accessibility events until ResumeAccessibilityEvents().
    // Calls to Pause nest; no events will be sent until the number of
    // Resume calls matches the number of Pause calls received.
    void PauseAccessibilityEvents()
    {
        accessibility_pause_level_++;
    }

    void ResumeAccessibilityEvents()
    {
        DCHECK(accessibility_pause_level_ > 0);
        accessibility_pause_level_--;
    }

    bool ShouldSendAccessibilityEvents()
    {
        return 0 == accessibility_pause_level_;
    }

private:
    bool restored_last_session_;

    // True only for the very first profile launched in a Chrome session.
    bool first_launched_;

    // Accessibility events will only be propagated when the pause
    // level is zero.  PauseAccessibilityEvents and ResumeAccessibilityEvents
    // increment and decrement the level, respectively, rather than set it to
    // true or false, so that calls can be nested.
    int accessibility_pause_level_;
};

#endif //__profile_h__