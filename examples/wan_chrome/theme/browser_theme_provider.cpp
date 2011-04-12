
#include "browser_theme_provider.h"

#include "base/file_path.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"

#include "message/task.h"

#include "gfx/color_utils.h"

#include "view_framework/base/app_res_ids.h"
#include "view_framework/base/resource_bundle.h"
#include "view_framework/widget/widget_win.h"

#include "../../wanui_res/resource.h"
#include "../common/notification_service.h"
#include "../profile/profile.h"
#include "../browser_thread.h"
#include "../chrome_constants.h"
#include "browser_theme_pack.h"

// Strings used in alignment properties.
const char* BrowserThemeProvider::kAlignmentTop = "top";
const char* BrowserThemeProvider::kAlignmentBottom = "bottom";
const char* BrowserThemeProvider::kAlignmentLeft = "left";
const char* BrowserThemeProvider::kAlignmentRight = "right";

// Strings used in background tiling repetition properties.
const char* BrowserThemeProvider::kTilingNoRepeat = "no-repeat";
const char* BrowserThemeProvider::kTilingRepeatX = "repeat-x";
const char* BrowserThemeProvider::kTilingRepeatY = "repeat-y";
const char* BrowserThemeProvider::kTilingRepeat = "repeat";

// The default theme if we haven't installed a theme yet or if we've clicked
// the "Use Classic" button.
const char* BrowserThemeProvider::kDefaultThemeID = "";

namespace
{

    // The default theme if we've gone to the theme gallery and installed the
    // "Default" theme. We have to detect this case specifically. (By the time we
    // realize we've installed the default theme, we already have an extension
    // unpacked on the filesystem.)
    const char* kDefaultThemeGalleryID = "hkacjpbfdknhflllbcmjibkdeoafencn";

    SkColor TintForUnderline(SkColor input)
    {
        return SkColorSetA(input, SkColorGetA(input)/3);
    }

    SkColor IncreaseLightness(SkColor color, double percent)
    {
        gfx::HSL result;
        gfx::SkColorToHSL(color, &result);
        result.l += (1 - result.l) * percent;
        return gfx::HSLToSkColor(result, SkColorGetA(color));
    }

    // Default colors.
    const SkColor kDefaultColorFrame = SkColorSetRGB(66, 116, 201);
    const SkColor kDefaultColorFrameInactive = SkColorSetRGB(161, 182, 228);
    const SkColor kDefaultColorFrameIncognito = SkColorSetRGB(83, 106, 139);
    const SkColor kDefaultColorFrameIncognitoInactive =
        SkColorSetRGB(126, 139, 156);
    const SkColor kDefaultColorToolbar = SkColorSetRGB(223, 223, 223);
    const SkColor kDefaultColorTabText = SK_ColorBLACK;
    const SkColor kDefaultColorBackgroundTabText = SkColorSetRGB(64, 64, 64);
    const SkColor kDefaultColorBookmarkText = SkColorSetRGB(18, 50, 114);
    const SkColor kDefaultColorNTPBackground = gfx::GetSysSkColor(COLOR_WINDOW);
    const SkColor kDefaultColorNTPText = gfx::GetSysSkColor(COLOR_WINDOWTEXT);
    const SkColor kDefaultColorNTPLink = gfx::GetSysSkColor(COLOR_HOTLIGHT);
    const SkColor kDefaultColorNTPHeader = SkColorSetRGB(150, 150, 150);
    const SkColor kDefaultColorNTPSection = SkColorSetRGB(229, 229, 229);
    const SkColor kDefaultColorNTPSectionText = SK_ColorBLACK;
    const SkColor kDefaultColorNTPSectionLink = SkColorSetRGB(6, 55, 116);
    const SkColor kDefaultColorControlBackground = SkColorSetARGB(0, 0, 0, 0);
    const SkColor kDefaultColorButtonBackground = SkColorSetARGB(0, 0, 0, 0);

    // Default tints.
    const gfx::HSL kDefaultTintButtons = { -1, -1, -1 };
    const gfx::HSL kDefaultTintFrame = { -1, -1, -1 };
    const gfx::HSL kDefaultTintFrameInactive = { -1, -1, 0.75f };
    const gfx::HSL kDefaultTintFrameIncognito = { -1, 0.2f, 0.35f };
    const gfx::HSL kDefaultTintFrameIncognitoInactive = { -1, 0.3f, 0.6f };
    const gfx::HSL kDefaultTintBackgroundTab = { -1, 0.5, 0.75 };

    // Default display properties.
    const int kDefaultDisplayPropertyNTPAlignment =
        BrowserThemeProvider::ALIGN_BOTTOM;
    const int kDefaultDisplayPropertyNTPTiling =
        BrowserThemeProvider::NO_REPEAT;
    const int kDefaultDisplayPropertyNTPInverseLogo = 0;

    // The sum of kFrameBorderThickness and kNonClientRestoredExtraThickness from
    // OpaqueBrowserFrameView.
    const int kRestoredTabVerticalOffset = 15;

    // The image resources we will allow people to theme.
    const int kThemeableImages[] =
    {
        IDR_THEME_FRAME,
        IDR_THEME_FRAME_INACTIVE,
        IDR_THEME_FRAME_INCOGNITO,
        IDR_THEME_FRAME_INCOGNITO_INACTIVE,
        IDR_THEME_TOOLBAR,
        IDR_THEME_TAB_BACKGROUND,
        IDR_THEME_TAB_BACKGROUND_INCOGNITO,
        IDR_THEME_TAB_BACKGROUND_V,
        IDR_THEME_NTP_BACKGROUND,
        IDR_THEME_FRAME_OVERLAY,
        IDR_THEME_FRAME_OVERLAY_INACTIVE,
        IDR_THEME_BUTTON_BACKGROUND,
        IDR_THEME_NTP_ATTRIBUTION,
        IDR_THEME_WINDOW_CONTROL_BACKGROUND
    };

    bool HasThemeableImage(int themeable_image_id)
    {
        static std::set<int> themeable_images;
        if(themeable_images.empty())
        {
            themeable_images.insert(kThemeableImages,
                kThemeableImages+arraysize(kThemeableImages));
        }
        return themeable_images.count(themeable_image_id) > 0;
    }

    // The image resources that will be tinted by the 'button' tint value.
    // If you change this list, you must increment the version number in
    // browser_theme_pack.cc, and you should assign persistent IDs to the
    // data table at the start of said file or else tinted versions of
    // these resources will not be created.
    const int kToolbarButtonIDs[] =
    {
        IDR_BACK, IDR_BACK_D, IDR_BACK_H, IDR_BACK_P,
        IDR_FORWARD, IDR_FORWARD_D, IDR_FORWARD_H, IDR_FORWARD_P,
        IDR_HOME, IDR_HOME_H, IDR_HOME_P,
        IDR_RELOAD, IDR_RELOAD_H, IDR_RELOAD_P,
        IDR_STOP, IDR_STOP_D, IDR_STOP_H, IDR_STOP_P,
        IDR_LOCATIONBG_C, IDR_LOCATIONBG_L, IDR_LOCATIONBG_R,
        IDR_BROWSER_ACTIONS_OVERFLOW, IDR_BROWSER_ACTIONS_OVERFLOW_H,
        IDR_BROWSER_ACTIONS_OVERFLOW_P,
        IDR_TOOLS, IDR_TOOLS_H, IDR_TOOLS_P,
        IDR_MENU_DROPARROW,
        IDR_THROBBER, IDR_THROBBER_WAITING, IDR_THROBBER_LIGHT,
    };

    // Writes the theme pack to disk on a separate thread.
    class WritePackToDiskTask : public Task
    {
    public:
        WritePackToDiskTask(BrowserThemePack* pack, const FilePath& path)
            : theme_pack_(pack), pack_path_(path) {}

        virtual void Run()
        {
            if(!theme_pack_->WriteToDisk(pack_path_))
            {
                NOTREACHED() << "Could not write theme pack to disk";
            }
        }

    private:
        scoped_refptr<BrowserThemePack> theme_pack_;
        FilePath pack_path_;
    };

}

bool BrowserThemeProvider::IsThemeableImage(int resource_id)
{
    return HasThemeableImage(resource_id);
}

BrowserThemeProvider::BrowserThemeProvider()
: rb_(ResourceBundle::GetSharedInstance()),
profile_(NULL)
{
    // Initialize the themeable image map so we can use it on other threads.
    HasThemeableImage(0);
}

BrowserThemeProvider::~BrowserThemeProvider()
{
    FreePlatformCaches();
}

void BrowserThemeProvider::Init(Profile* profile)
{
    DCHECK(CalledOnValidThread());
    profile_ = profile;

    LoadThemePrefs();
}

SkBitmap* BrowserThemeProvider::GetBitmapNamed(int id) const
{
    DCHECK(CalledOnValidThread());

    SkBitmap* bitmap = NULL;

    if(theme_pack_.get())
    {
        bitmap = theme_pack_->GetBitmapNamed(id);
    }

    if(!bitmap)
    {
        bitmap = rb_.GetBitmapNamed(id);
    }

    return bitmap;
}

SkColor BrowserThemeProvider::GetColor(int id) const
{
    DCHECK(CalledOnValidThread());

    SkColor color;
    if(theme_pack_.get() && theme_pack_->GetColor(id, &color))
    {
        return color;
    }

    // For backward compat with older themes, some newer colors are generated from
    // older ones if they are missing.
    switch(id)
    {
    case COLOR_NTP_SECTION_HEADER_TEXT:
        return IncreaseLightness(GetColor(COLOR_NTP_TEXT), 0.30);
    case COLOR_NTP_SECTION_HEADER_TEXT_HOVER:
        return GetColor(COLOR_NTP_TEXT);
    case COLOR_NTP_SECTION_HEADER_RULE:
        return IncreaseLightness(GetColor(COLOR_NTP_TEXT), 0.70);
    case COLOR_NTP_SECTION_HEADER_RULE_LIGHT:
        return IncreaseLightness(GetColor(COLOR_NTP_TEXT), 0.86);
    case COLOR_NTP_TEXT_LIGHT:
        return IncreaseLightness(GetColor(COLOR_NTP_TEXT), 0.40);
    }

    return GetDefaultColor(id);
}

bool BrowserThemeProvider::GetDisplayProperty(int id, int* result) const
{
    if(theme_pack_.get())
    {
        return theme_pack_->GetDisplayProperty(id, result);
    }

    return GetDefaultDisplayProperty(id, result);
}

bool BrowserThemeProvider::ShouldUseNativeFrame() const
{
    if(HasCustomImage(IDR_THEME_FRAME))
    {
        return false;
    }
    return view::WidgetWin::IsAeroGlassEnabled();
}

bool BrowserThemeProvider::HasCustomImage(int id) const
{
    if(!HasThemeableImage(id))
    {
        return false;
    }

    if(theme_pack_)
    {
        return theme_pack_->HasCustomImage(id);
    }

    return false;
}

base::RefCountedMemory* BrowserThemeProvider::GetRawData(int id) const
{
    // Check to see whether we should substitute some images.
    int ntp_alternate;
    GetDisplayProperty(NTP_LOGO_ALTERNATE, &ntp_alternate);
    if(id==IDR_PRODUCT_LOGO && ntp_alternate!=0)
    {
        id = IDR_PRODUCT_LOGO_WHITE;
    }

    base::RefCountedMemory* data = NULL;
    if(theme_pack_.get())
    {
        data = theme_pack_->GetRawData(id);
    }
    if(!data)
    {
        data = rb_.LoadDataResourceBytes(id);
    }

    return data;
}

void BrowserThemeProvider::SetTheme(const Extension* extension)
{
    // Clear our image cache.
    FreePlatformCaches();

    DCHECK(extension);
    DCHECK(extension->is_theme());

    BuildFromExtension(extension);
    SaveThemeID(extension->id());

    NotifyThemeChanged(extension);
}

void BrowserThemeProvider::UseDefaultTheme()
{
    ClearAllThemeData();
    NotifyThemeChanged(NULL);
}

void BrowserThemeProvider::SetNativeTheme()
{
    UseDefaultTheme();
}

bool BrowserThemeProvider::UsingDefaultTheme()
{
    std::string id = GetThemeID();
    return id==BrowserThemeProvider::kDefaultThemeID ||
        id==kDefaultThemeGalleryID;
}

// static
std::string BrowserThemeProvider::AlignmentToString(int alignment)
{
    // Convert from an AlignmentProperty back into a string.
    std::string vertical_string;
    std::string horizontal_string;

    if(alignment & BrowserThemeProvider::ALIGN_TOP)
    {
        vertical_string = kAlignmentTop;
    }
    else if(alignment & BrowserThemeProvider::ALIGN_BOTTOM)
    {
        vertical_string = kAlignmentBottom;
    }

    if(alignment & BrowserThemeProvider::ALIGN_LEFT)
    {
        horizontal_string = kAlignmentLeft;
    }
    else if(alignment & BrowserThemeProvider::ALIGN_RIGHT)
    {
        horizontal_string = kAlignmentRight;
    }

    if(vertical_string.empty())
    {
        return horizontal_string;
    }
    if(horizontal_string.empty())
    {
        return vertical_string;
    }
    return vertical_string + " " + horizontal_string;
}

// static
int BrowserThemeProvider::StringToAlignment(const std::string& alignment)
{
    std::vector<std::wstring> split;
    SplitStringAlongWhitespace(UTF8ToWide(alignment), &split);

    int alignment_mask = 0;
    for(std::vector<std::wstring>::iterator alignments(split.begin());
        alignments!=split.end(); ++alignments)
    {
        std::string comp = WideToUTF8(*alignments);
        const char* component = comp.c_str();

        if(base::strcasecmp(component, kAlignmentTop) == 0)
        {
            alignment_mask |= BrowserThemeProvider::ALIGN_TOP;
        }
        else if(base::strcasecmp(component, kAlignmentBottom) == 0)
        {
            alignment_mask |= BrowserThemeProvider::ALIGN_BOTTOM;
        }

        if(base::strcasecmp(component, kAlignmentLeft) == 0)
        {
            alignment_mask |= BrowserThemeProvider::ALIGN_LEFT;
        }
        else if(base::strcasecmp(component, kAlignmentRight) == 0)
        {
            alignment_mask |= BrowserThemeProvider::ALIGN_RIGHT;
        }
    }
    return alignment_mask;
}

// static
std::string BrowserThemeProvider::TilingToString(int tiling)
{
    // Convert from a TilingProperty back into a string.
    if(tiling == BrowserThemeProvider::REPEAT_X)
    {
        return kTilingRepeatX;
    }
    if(tiling == BrowserThemeProvider::REPEAT_Y)
    {
        return kTilingRepeatY;
    }
    if(tiling == BrowserThemeProvider::REPEAT)
    {
        return kTilingRepeat;
    }
    return kTilingNoRepeat;
}

// static
int BrowserThemeProvider::StringToTiling(const std::string& tiling)
{
    const char* component = tiling.c_str();

    if(base::strcasecmp(component, kTilingRepeatX) == 0)
    {
        return BrowserThemeProvider::REPEAT_X;
    }
    if(base::strcasecmp(component, kTilingRepeatY) == 0)
    {
        return BrowserThemeProvider::REPEAT_Y;
    }
    if(base::strcasecmp(component, kTilingRepeat) == 0)
    {
        return BrowserThemeProvider::REPEAT;
    }
    // NO_REPEAT is the default choice.
    return BrowserThemeProvider::NO_REPEAT;
}

// static
gfx::HSL BrowserThemeProvider::GetDefaultTint(int id)
{
    switch(id)
    {
    case TINT_FRAME:
        return kDefaultTintFrame;
    case TINT_FRAME_INACTIVE:
        return kDefaultTintFrameInactive;
    case TINT_FRAME_INCOGNITO:
        return kDefaultTintFrameIncognito;
    case TINT_FRAME_INCOGNITO_INACTIVE:
        return kDefaultTintFrameIncognitoInactive;
    case TINT_BUTTONS:
        return kDefaultTintButtons;
    case TINT_BACKGROUND_TAB:
        return kDefaultTintBackgroundTab;
    default:
        gfx::HSL result = { -1, -1, -1 };
        return result;
    }
}

// static
SkColor BrowserThemeProvider::GetDefaultColor(int id)
{
    switch(id)
    {
    case COLOR_FRAME:
        return kDefaultColorFrame;
    case COLOR_FRAME_INACTIVE:
        return kDefaultColorFrameInactive;
    case COLOR_FRAME_INCOGNITO:
        return kDefaultColorFrameIncognito;
    case COLOR_FRAME_INCOGNITO_INACTIVE:
        return kDefaultColorFrameIncognitoInactive;
    case COLOR_TOOLBAR:
        return kDefaultColorToolbar;
    case COLOR_TAB_TEXT:
        return kDefaultColorTabText;
    case COLOR_BACKGROUND_TAB_TEXT:
        return kDefaultColorBackgroundTabText;
    case COLOR_BOOKMARK_TEXT:
        return kDefaultColorBookmarkText;
    case COLOR_NTP_BACKGROUND:
        return kDefaultColorNTPBackground;
    case COLOR_NTP_TEXT:
        return kDefaultColorNTPText;
    case COLOR_NTP_LINK:
        return kDefaultColorNTPLink;
    case COLOR_NTP_LINK_UNDERLINE:
        return TintForUnderline(kDefaultColorNTPLink);
    case COLOR_NTP_HEADER:
        return kDefaultColorNTPHeader;
    case COLOR_NTP_SECTION:
        return kDefaultColorNTPSection;
    case COLOR_NTP_SECTION_TEXT:
        return kDefaultColorNTPSectionText;
    case COLOR_NTP_SECTION_LINK:
        return kDefaultColorNTPSectionLink;
    case COLOR_NTP_SECTION_LINK_UNDERLINE:
        return TintForUnderline(kDefaultColorNTPSectionLink);
    case COLOR_CONTROL_BACKGROUND:
        return kDefaultColorControlBackground;
    case COLOR_BUTTON_BACKGROUND:
        return kDefaultColorButtonBackground;
    default:
        // Return a debugging red color.
        return 0xffff0000;
    }
}

// static
bool BrowserThemeProvider::GetDefaultDisplayProperty(int id, int* result)
{
    switch(id)
    {
    case NTP_BACKGROUND_ALIGNMENT:
        *result = kDefaultDisplayPropertyNTPAlignment;
        return true;
    case NTP_BACKGROUND_TILING:
        *result = kDefaultDisplayPropertyNTPTiling;
        return true;
    case NTP_LOGO_ALTERNATE:
        *result = kDefaultDisplayPropertyNTPInverseLogo;
        return true;
    }

    return false;
}

// static
const std::set<int>& BrowserThemeProvider::GetTintableToolbarButtons()
{
    static std::set<int> button_set;
    if(button_set.empty())
    {
        button_set = std::set<int>(kToolbarButtonIDs,
            kToolbarButtonIDs+arraysize(kToolbarButtonIDs));
    }

    return button_set;
}

gfx::HSL BrowserThemeProvider::GetTint(int id) const
{
    DCHECK(CalledOnValidThread());

    gfx::HSL hsl;
    if(theme_pack_.get() && theme_pack_->GetTint(id, &hsl))
    {
        return hsl;
    }

    return GetDefaultTint(id);
}

void BrowserThemeProvider::ClearAllThemeData()
{
    // Clear our image cache.
    FreePlatformCaches();
    theme_pack_ = NULL;

    profile_->GetPrefs()->ClearPref(kCurrentThemePackFilename);
    SaveThemeID(kDefaultThemeID);
}

void BrowserThemeProvider::LoadThemePrefs()
{
    PrefService* prefs = profile_->GetPrefs();

    std::string current_id = GetThemeID();
    if(current_id != kDefaultThemeID)
    {
        bool loaded_pack = false;

        // If we don't have a file pack, we're updating from an old version.
        FilePath path = prefs->GetFilePath(kCurrentThemePackFilename);
        if(path != FilePath())
        {
            theme_pack_ = BrowserThemePack::BuildFromDataPack(path, current_id);
            loaded_pack = theme_pack_.get() != NULL;
        }
    }
}

void BrowserThemeProvider::NotifyThemeChanged(const Extension* extension)
{
    VLOG(1) << "Sending BROWSER_THEME_CHANGED";
    // Redraw!
    NotificationService* service = NotificationService::current();
    service->Notify(NotificationType::BROWSER_THEME_CHANGED,
        Source<BrowserThemeProvider>(this),
        Details<const Extension>(extension));
}

void BrowserThemeProvider::FreePlatformCaches()
{
    // Views (Skia) has no platform image cache to clear.
}

void BrowserThemeProvider::SavePackName(const FilePath& pack_path)
{
    profile_->GetPrefs()->SetFilePath(kCurrentThemePackFilename,
        pack_path);
}

void BrowserThemeProvider::SaveThemeID(const std::string& id)
{
    profile_->GetPrefs()->SetString(kCurrentThemeID, id);
}

void BrowserThemeProvider::BuildFromExtension(const Extension* extension)
{
    scoped_refptr<BrowserThemePack> pack(
        BrowserThemePack::BuildFromExtension(extension));
    if(!pack.get())
    {
        // TODO(erg): We've failed to install the theme; perhaps we should tell the
        // user? http://crbug.com/34780
        LOG(ERROR) << "Could not load theme.";
        return;
    }

    // Write the packed file to disk.
    FilePath pack_path = extension->path().Append(kThemePackFilename);
    BrowserThread::PostTask(BrowserThread::FILE,
        new WritePackToDiskTask(pack, pack_path));

    SavePackName(pack_path);
    theme_pack_ = pack;
}