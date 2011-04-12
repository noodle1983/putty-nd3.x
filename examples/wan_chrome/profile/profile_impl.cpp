
#include "profile_impl.h"

#include "base/command_line.h"
#include "base/file_util.h"

#include "view/base/resource_bundle.h"

#include "../common/notification_service.h"
#include "../theme/browser_theme_provider.h"
#include "../browser_thread.h"
#include "../chrome_constants.h"

using base::Time;
using base::TimeDelta;

namespace
{

    // Delay, in milliseconds, before we explicitly create the SessionService.
    static const int kCreateSessionServiceDelayMS = 500;

    // Simple task to log the size of the current profile.
    class ProfileSizeTask : public Task
    {
    public:
        explicit ProfileSizeTask(const FilePath& path) : path_(path) {}
        virtual ~ProfileSizeTask() {}

        virtual void Run();

    private:
        FilePath path_;
    };

    void ProfileSizeTask::Run()
    {
        int64 size = base::ComputeFilesSize(path_, FILE_PATH_LITERAL("*"));
        int size_MB = static_cast<int>(size  / (1024 * 1024));

        size = base::ComputeFilesSize(path_, FILE_PATH_LITERAL("History"));
        size_MB = static_cast<int>(size  / (1024 * 1024));

        size = base::ComputeFilesSize(path_, FILE_PATH_LITERAL("History*"));
        size_MB = static_cast<int>(size  / (1024 * 1024));

        size = base::ComputeFilesSize(path_, FILE_PATH_LITERAL("Cookies"));
        size_MB = static_cast<int>(size  / (1024 * 1024));

        size = base::ComputeFilesSize(path_, FILE_PATH_LITERAL("Bookmarks"));
        size_MB = static_cast<int>(size  / (1024 * 1024));

        size = base::ComputeFilesSize(path_, FILE_PATH_LITERAL("Favicons"));
        size_MB = static_cast<int>(size  / (1024 * 1024));

        size = base::ComputeFilesSize(path_, FILE_PATH_LITERAL("Top Sites"));
        size_MB = static_cast<int>(size  / (1024 * 1024));

        size = base::ComputeFilesSize(path_, FILE_PATH_LITERAL("Visited Links"));
        size_MB = static_cast<int>(size  / (1024 * 1024));

        size = base::ComputeFilesSize(path_, FILE_PATH_LITERAL("Web Data"));
        size_MB = static_cast<int>(size  / (1024 * 1024));

        size = base::ComputeFilesSize(path_, FILE_PATH_LITERAL("Extension*"));
        size_MB = static_cast<int>(size  / (1024 * 1024));
    }

}

// static
Profile* Profile::CreateProfile(const FilePath& path)
{
    return new ProfileImpl(path);
}


ProfileImpl::ProfileImpl(const FilePath& path)
: path_(path),
created_theme_provider_(false),
start_time_(Time::Now())
{
    DCHECK(!path.empty()) << "Using an empty path will attempt to write " <<
        "profile files to the root directory!";

    base_cache_path_ = path_;
    base::CreateDirectory(base_cache_path_);

    // Listen for theme installations from our original profile.
    registrar_.Add(this, NotificationType::THEME_INSTALLED,
        Source<Profile>(GetOriginalProfile()));

    // Listen for bookmark model load, to bootstrap the sync service.
    // On CrOS sync service will be initialized after sign in.
    registrar_.Add(this, NotificationType::BOOKMARK_MODEL_LOADED,
        Source<Profile>(this));

    // Log the profile size after a reasonable startup delay.
    BrowserThread::PostDelayedTask(BrowserThread::FILE,
        new ProfileSizeTask(path_), 112000);
}

ProfileImpl::~ProfileImpl()
{
    NotificationService::current()->Notify(
        NotificationType::PROFILE_DESTROYED,
        Source<Profile>(this),
        NotificationService::NoDetails());

    // The theme provider provides bitmaps to whoever wants them.
    theme_provider_.reset();
}

ProfileId ProfileImpl::GetRuntimeId()
{
    return reinterpret_cast<ProfileId>(this);
}

FilePath ProfileImpl::GetPath()
{
    return path_;
}

Profile* ProfileImpl::GetOriginalProfile()
{
    return this;
}

void ProfileImpl::InitThemes()
{
    if(!created_theme_provider_)
    {
        theme_provider_.reset(new BrowserThemeProvider);
        theme_provider_->Init(this);
        created_theme_provider_ = true;
    }
}

void ProfileImpl::SetTheme(const Extension* extension)
{
    InitThemes();
    theme_provider_.get()->SetTheme(extension);
}

void ProfileImpl::SetNativeTheme()
{
    InitThemes();
    theme_provider_.get()->SetNativeTheme();
}

void ProfileImpl::ClearTheme()
{
    InitThemes();
    theme_provider_.get()->UseDefaultTheme();
}

const Extension* ProfileImpl::GetTheme()
{
    InitThemes();

    std::string id = theme_provider_.get()->GetThemeID();
    if(id == BrowserThemeProvider::kDefaultThemeID)
    {
        return NULL;
    }

    return extensions_service_->GetExtensionById(id, false);
}

BrowserThemeProvider* ProfileImpl::GetThemeProvider()
{
    InitThemes();
    return theme_provider_.get();
}

bool ProfileImpl::IsSameProfile(Profile* profile)
{
    if(profile == static_cast<Profile*>(this))
    {
        return true;
    }
    return false;
}

Time ProfileImpl::GetStartTime() const
{
    return start_time_;
}

void ProfileImpl::Observe(NotificationType type,
                          const NotificationSource& source,
                          const NotificationDetails& details)
{
}

FilePath ProfileImpl::GetPrefFilePath()
{
    FilePath pref_file_path = path_;
    pref_file_path = pref_file_path.Append(kPreferencesFilename);
    return pref_file_path;
}