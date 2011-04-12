
#ifndef __view_framework_resource_bundle_h__
#define __view_framework_resource_bundle_h__

#pragma once

#include <map>

#include "base/file_path.h"
#include "base/ref_counted_memory.h"
#include "base/scoped_ptr.h"
#include "base/string16.h"

namespace base
{
    class DataPack;
    class Lock;
    class StringPiece;
}

namespace gfx
{
    class Font;
    class Image;
}

class SkBitmap;
typedef uint32 SkColor;

// ResourceBundle is a central facility to load images and other resources,
// such as theme graphics.
// Every resource is loaded only once.
class ResourceBundle
{
public:
    // An enumeration of the various font styles used throughout Chrome.
    // The following holds true for the font sizes:
    // Small <= Base <= Bold <= Medium <= MediumBold <= Large.
    enum FontStyle
    {
        SmallFont,
        BaseFont,
        BoldFont,
        MediumFont,
        // NOTE: depending upon the locale, this may *not* result in a bold font.
        MediumBoldFont,
        LargeFont,
    };

    // Initialize the ResourceBundle for this process.  Returns the language
    // selected.
    // NOTE: Mac ignores this and always loads up resources for the language
    // defined by the Cocoa UI (ie-NSBundle does the langange work).
    static bool InitSharedInstance(const FilePath& pref_locale);

    // Changes the locale for an already-initialized ResourceBundle.  Future
    // calls to get strings will return the strings for this new locale.  This
    // has no effect on existing or future image resources.  This has no effect
    // on existing or future image resources, and thus does not use the lock to
    // guarantee thread-safety, since all string access is expected to happen on
    // the UI thread.
    static bool ReloadSharedInstance(const FilePath& pref_locale);

    // Registers additional data pack files with the global ResourceBundle.  When
    // looking for a DataResource, we will search these files after searching the
    // main module.  This method is not thread safe!  You should call it
    // immediately after calling InitSharedInstance.
    static void AddDataPackToSharedInstance(const FilePath& path);

    // Delete the ResourceBundle for this process if it exists.
    static void CleanupSharedInstance();

    // Return the global resource loader instance.
    static ResourceBundle& GetSharedInstance();

    // Gets the bitmap with the specified resource_id from the current module
    // data. Returns a pointer to a shared instance of the SkBitmap. This shared
    // bitmap is owned by the resource bundle and should not be freed.
    //
    // The bitmap is assumed to exist. This function will log in release, and
    // assert in debug mode if it does not. On failure, this will return a
    // pointer to a shared empty placeholder bitmap so it will be visible what
    // is missing.
    SkBitmap* GetBitmapNamed(int resource_id);

    // Gets an image resource from the current module data. This will load the
    // image in Skia format by default. The ResourceBundle owns this.
    gfx::Image& GetImageNamed(int resource_id);

    // Similar to GetImageNamed, but rather than loading the image in Skia format,
    // it will load in the native platform type. This can avoid conversion from
    // one image type to another. ResourceBundle owns the result.
    //
    // Note that if the same resource has already been loaded in GetImageNamed(),
    // gfx::Image will perform a conversion, rather than using the native image
    // loading code of ResourceBundle.
    gfx::Image& GetNativeImageNamed(int resource_id);

    // Loads the raw bytes of a data resource into |bytes|,
    // without doing any processing or interpretation of
    // the resource. Returns whether we successfully read the resource.
    base::RefCountedStaticMemory* LoadDataResourceBytes(int resource_id) const;

    // Return the contents of a resource in a StringPiece given the resource id.
    base::StringPiece GetRawDataResource(int resource_id) const;

    // Get a localized string given a message id.  Returns an empty
    // string if the message_id is not found.
    string16 GetLocalizedString(int message_id);

    // Returns the font for the specified style.
    const gfx::Font& GetFont(FontStyle style);

    // Loads and returns an icon from the app module.
    HICON LoadThemeIcon(int icon_id);

    // Loads and returns a cursor from the app module.
    HCURSOR LoadCursor(int cursor_id);

    // TODO(glen): Move these into theme provider (dialogs still depend on
    //    ResourceBundle).
    static const SkColor frame_color;
    static const SkColor frame_color_inactive;
    static const SkColor frame_color_app_panel;
    static const SkColor frame_color_app_panel_inactive;
    static const SkColor frame_color_incognito;
    static const SkColor frame_color_incognito_inactive;
    static const SkColor toolbar_color;
    static const SkColor toolbar_separator_color;

    //enum AppId
    //{
    //    BITMAP_FRAME = 0,
    //    BITMAP_FRAME_INACTIVE,
    //    BITMAP_WINDOW_TOP_LEFT_CORNER,
    //    BITMAP_WINDOW_TOP_CENTER,
    //    BITMAP_WINDOW_TOP_RIGHT_CORNER,
    //    BITMAP_WINDOW_LEFT_SIDE,
    //    BITMAP_WINDOW_RIGHT_SIDE,
    //    BITMAP_WINDOW_BOTTOM_LEFT_CORNER,
    //    BITMAP_WINDOW_BOTTOM_CENTER,
    //    BITMAP_WINDOW_BOTTOM_RIGHT_CORNER,
    //    BITMAP_APP_TOP_LEFT,
    //    BITMAP_APP_TOP_CENTER,
    //    BITMAP_APP_TOP_RIGHT,
    //    BITMAP_CONTENT_BOTTOM_LEFT_CORNER,
    //    BITMAP_CONTENT_BOTTOM_CENTER,
    //    BITMAP_CONTENT_BOTTOM_RIGHT_CORNER,
    //    BITMAP_CONTENT_LEFT_SIDE,
    //    BITMAP_CONTENT_RIGHT_SIDE,
    //    BITMAP_CLOSE,
    //    BITMAP_CLOSE_SA,
    //    BITMAP_CLOSE_H,
    //    BITMAP_CLOSE_SA_H,
    //    BITMAP_CLOSE_P,
    //    BITMAP_CLOSE_SA_P,
    //    BITMAP_RESTORE,
    //    BITMAP_RESTORE_H,
    //    BITMAP_RESTORE_P,
    //    BITMAP_MAXIMIZE,
    //    BITMAP_MAXIMIZE_H,
    //    BITMAP_MAXIMIZE_P,
    //    BITMAP_MINIMIZE,
    //    BITMAP_MINIMIZE_H,
    //    BITMAP_MINIMIZE_P,

    //    BITMAP_TEXTBUTTON_TOP_LEFT_H,
    //    BITMAP_TEXTBUTTON_TOP_H,
    //    BITMAP_TEXTBUTTON_TOP_RIGHT_H,
    //    BITMAP_TEXTBUTTON_LEFT_H,
    //    BITMAP_TEXTBUTTON_CENTER_H,
    //    BITMAP_TEXTBUTTON_RIGHT_H,
    //    BITMAP_TEXTBUTTON_BOTTOM_LEFT_H,
    //    BITMAP_TEXTBUTTON_BOTTOM_H,
    //    BITMAP_TEXTBUTTON_BOTTOM_RIGHT_H,
    //    BITMAP_TEXTBUTTON_TOP_LEFT_P,
    //    BITMAP_TEXTBUTTON_TOP_P,
    //    BITMAP_TEXTBUTTON_TOP_RIGHT_P,
    //    BITMAP_TEXTBUTTON_LEFT_P,
    //    BITMAP_TEXTBUTTON_CENTER_P,
    //    BITMAP_TEXTBUTTON_RIGHT_P,
    //    BITMAP_TEXTBUTTON_BOTTOM_LEFT_P,
    //    BITMAP_TEXTBUTTON_BOTTOM_P,
    //    BITMAP_TEXTBUTTON_BOTTOM_RIGHT_P,

    //    BITMAP_THROBBER,
    //    BITMAP_INPUT_GOOD,

    //    BITMAP_MENU_DROPARROW,


    //    STRING_UI_FONT_FAMILY,
    //    STRING_UI_FONT_FAMILY_XP,
    //    STRING_UI_FONT_SIZE_SCALER,
    //    STRING_UI_FONT_SIZE_SCALER_XP,
    //    STRING_MINIMUM_UI_FONT_SIZE,

    //    STRING_SCROLLBAR_CXMENU_SCROLLHERE,
    //    STRING_SCROLLBAR_CXMENU_SCROLLLEFTEDGE,
    //    STRING_SCROLLBAR_CXMENU_SCROLLHOME,
    //    STRING_SCROLLBAR_CXMENU_SCROLLRIGHTEDGE,
    //    STRING_SCROLLBAR_CXMENU_SCROLLEND,
    //    STRING_SCROLLBAR_CXMENU_SCROLLPAGEUP,
    //    STRING_SCROLLBAR_CXMENU_SCROLLPAGEDOWN,
    //    STRING_SCROLLBAR_CXMENU_SCROLLLEFT,
    //    STRING_SCROLLBAR_CXMENU_SCROLLUP,
    //    STRING_SCROLLBAR_CXMENU_SCROLLRIGHT,
    //    STRING_SCROLLBAR_CXMENU_SCROLLDOWN,

    //    STRING_ACCNAME_MINIMIZE,
    //    STRING_ACCNAME_MAXIMIZE,
    //    STRING_ACCNAME_RESTORE,
    //    STRING_ACCNAME_CLOSE,

    //    STRING_OK,
    //    STRING_CANCEL,
    //    STRING_CLOSE,

    //    STRING_ACCACTION_PRESS,

    //    STRING_TAB_KEY,
    //    STRING_ENTER_KEY,
    //    STRING_ESC_KEY,
    //    STRING_PAGEUP_KEY,
    //    STRING_PAGEDOWN_KEY,
    //    STRING_END_KEY,
    //    STRING_HOME_KEY,
    //    STRING_INSERT_KEY,
    //    STRING_DELETE_KEY,
    //    STRING_LEFT_ARROW_KEY,
    //    STRING_RIGHT_ARROW_KEY,
    //    STRING_BACKSPACE_KEY,
    //    STRING_F1_KEY,
    //    STRING_F11_KEY,
    //    STRING_SHIFT_MODIFIER,
    //    STRING_CONTROL_MODIFIER,
    //    STRING_ALT_MODIFIER,

    //    STRING_MENU_EMPTY_SUBMENU,


    //    APPID__COUNT,
    //};
    //class IdConveter
    //{
    //public:
    //    IdConveter() {}
    //    virtual ~IdConveter() {}

    //    virtual int AppIdToResId(AppId app_id) = 0;
    //};
    //void SetIdConveter(IdConveter* conveter);
    //int ConvetAppId(AppId app_id) const;
    //SkBitmap* GetBitmapAppNamed(AppId app_id);
    //string16 GetLocalizedAppString(AppId app_id);

private:
    // Helper class for managing data packs.
    class LoadedDataPack
    {
    public:
        explicit LoadedDataPack(const FilePath& path);
        ~LoadedDataPack();
        bool GetStringPiece(int resource_id, base::StringPiece* data) const;
        base::RefCountedStaticMemory* GetStaticMemory(int resource_id) const;

    private:
        void Load();

        scoped_ptr<base::DataPack> data_pack_;
        FilePath path_;

        DISALLOW_COPY_AND_ASSIGN(LoadedDataPack);
    };

    // We define a DataHandle typedef to abstract across how data is stored
    // across platforms.
    // Windows stores resources in DLLs, which are managed by HINSTANCE.
    typedef HINSTANCE DataHandle;

    // Ctor/dtor are private, since we're a singleton.
    ResourceBundle();
    ~ResourceBundle();

    // Free skia_images_.
    void FreeImages();

    // Load the main resources.
    void LoadCommonResources();

    // Try to load the locale specific strings from an external data module.
    // Returns the locale that is loaded.
    bool LoadLocaleResources(const FilePath& pref_locale);

    // Unload the locale specific strings and prepares to load new ones. See
    // comments for ReloadSharedInstance().
    void UnloadLocaleResources();

    // Initialize all the gfx::Font members if they haven't yet been initialized.
    void LoadFontsIfNecessary();

    // Returns a handle to bytes from the resource |module|, without doing any
    // processing or interpretation of the resource. Returns whether we
    // successfully read the resource.  Caller does not own the data returned
    // through this method and must not modify the data pointed to by |bytes|.
    static base::RefCountedStaticMemory* LoadResourceBytes(DataHandle module,
        int resource_id);

    // Creates and returns a new SkBitmap given the data file to look in and the
    // resource id.  It's up to the caller to free the returned bitmap when
    // done.
    static SkBitmap* LoadBitmap(DataHandle dll_inst, int resource_id);

    // Returns an empty image for when a resource cannot be loaded. This is a
    // bright red bitmap.
    gfx::Image* GetEmptyImage();

    // Class level lock.  Used to protect internal data structures that may be
    // accessed from other threads (e.g., skia_images_).
    scoped_ptr<base::Lock> lock_;

    // Handles for data sources.
    DataHandle resources_data_;
    DataHandle locale_resources_data_;

    // References to extra data packs loaded via AddDataPackToSharedInstance.
    std::vector<LoadedDataPack*> data_packs_;

    // Cached images. The ResourceBundle caches all retrieved images and keeps
    // ownership of the pointers.
    typedef std::map<int, gfx::Image*> ImageMap;
    ImageMap images_;

    // The various fonts used. Cached to avoid repeated GDI creation/destruction.
    scoped_ptr<gfx::Font> base_font_;
    scoped_ptr<gfx::Font> bold_font_;
    scoped_ptr<gfx::Font> small_font_;
    scoped_ptr<gfx::Font> medium_font_;
    scoped_ptr<gfx::Font> medium_bold_font_;
    scoped_ptr<gfx::Font> large_font_;
    scoped_ptr<gfx::Font> web_font_;

    //scoped_ptr<IdConveter> id_conveter_;

    static ResourceBundle* g_shared_instance_;

    DISALLOW_COPY_AND_ASSIGN(ResourceBundle);
};

#endif //__view_framework_resource_bundle_h__