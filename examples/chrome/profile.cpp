
#include "profile.h"

#include "base/file_path.h"
#include "base/time.h"

using base::Time;
using base::TimeDelta;

namespace
{

    // Used to tag the first profile launched in a Chrome session.
    bool g_first_profile_launched = true;

}

Profile::Profile()
: restored_last_session_(false),
first_launched_(g_first_profile_launched),
accessibility_pause_level_(0)
{
    g_first_profile_launched = false;
}

// static
const char* const Profile::kProfileKey = "__PROFILE__";

// static
const LocalProfileId Profile::kInvalidLocalProfileId =
static_cast<LocalProfileId>(0);

// static
void Profile::RegisterUserPrefs(PrefService* prefs)
{
    //prefs->RegisterBooleanPref(prefs::kSearchSuggestEnabled,
    //    true,
    //    PrefService::SYNCABLE_PREF);
    //prefs->RegisterBooleanPref(prefs::kSessionExitedCleanly,
    //    true,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterBooleanPref(prefs::kSafeBrowsingEnabled,
    //    true,
    //    PrefService::SYNCABLE_PREF);
    //prefs->RegisterBooleanPref(prefs::kSafeBrowsingReportingEnabled,
    //    false,
    //    PrefService::UNSYNCABLE_PREF);
    //// TODO(estade): IDS_SPELLCHECK_DICTIONARY should be an ASCII string.
    //prefs->RegisterLocalizedStringPref(prefs::kSpellCheckDictionary,
    //    IDS_SPELLCHECK_DICTIONARY,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterBooleanPref(prefs::kSpellCheckUseSpellingService,
    //    true,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterBooleanPref(prefs::kEnableSpellCheck,
    //    true,
    //    PrefService::SYNCABLE_PREF);
    //prefs->RegisterBooleanPref(prefs::kEnableAutoSpellCorrect,
    //    true,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterBooleanPref(prefs::kSpeechInputCensorResults,
    //    true,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterFilePathPref(prefs::kCurrentThemePackFilename,
    //    FilePath(),
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterStringPref(prefs::kCurrentThemeID,
    //    ThemeService::kDefaultThemeID,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterDictionaryPref(prefs::kCurrentThemeImages,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterDictionaryPref(prefs::kCurrentThemeColors,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterDictionaryPref(prefs::kCurrentThemeTints,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterDictionaryPref(prefs::kCurrentThemeDisplayProperties,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterBooleanPref(prefs::kDisableExtensions,
    //    false,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterStringPref(prefs::kSelectFileLastDirectory,
    //    "",
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterDoublePref(prefs::kDefaultZoomLevel,
    //    0.0,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterDictionaryPref(prefs::kPerHostZoomLevels,
    //    PrefService::UNSYNCABLE_PREF);
    //prefs->RegisterBooleanPref(prefs::kSyncPromoExpanded,
    //    true,
    //    PrefService::UNSYNCABLE_PREF);
}

std::string Profile::GetDebugName()
{
    std::string name = GetPath().BaseName().MaybeAsASCII();
    if(name.empty())
    {
        name = "UnknownProfile";
    }
    return name;
}

// static
bool Profile::IsGuestSession()
{
    return false;
}