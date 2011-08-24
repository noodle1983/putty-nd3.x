
#include "resource_bundle.h"

#include "base/debug/stack_trace.h"
#include "base/logging.h"
#include "base/resource_util.h"
#include "base/stl_utilinl.h"
#include "base/string_piece.h"
#include "base/synchronization/lock.h"
#include "base/win/windows_version.h"

#include "SkBitmap.h"

#include "ui_gfx/codec/png_codec.h"
#include "ui_gfx/font.h"
#include "ui_gfx/image/image.h"

#include "data_pack.h"

namespace ui
{

    namespace
    {
        const int kSmallFontSizeDelta = -2;
        const int kMediumFontSizeDelta = 3;
        const int kLargeFontSizeDelta = 8;
    }

    ResourceBundle* ResourceBundle::g_shared_instance_ = NULL;

    /* static */
    // TODO(glen): Finish moving these into theme provider (dialogs still
    //    depend on these colors).
    const SkColor ResourceBundle::frame_color =
        SkColorSetRGB(66, 116, 201);
    const SkColor ResourceBundle::frame_color_inactive =
        SkColorSetRGB(161, 182, 228);
    const SkColor ResourceBundle::frame_color_app_panel =
        SK_ColorWHITE;
    const SkColor ResourceBundle::frame_color_app_panel_inactive =
        SK_ColorWHITE;
    const SkColor ResourceBundle::frame_color_incognito =
        SkColorSetRGB(83, 106, 139);
    const SkColor ResourceBundle::frame_color_incognito_inactive =
        SkColorSetRGB(126, 139, 156);
    const SkColor ResourceBundle::toolbar_color =
        SkColorSetRGB(210, 225, 246);
    const SkColor ResourceBundle::toolbar_separator_color =
        SkColorSetRGB(182, 186, 192);

    /* static */
    bool ResourceBundle::InitSharedInstance(
        const FilePath& pref_locale)
    {
        DCHECK(g_shared_instance_ == NULL) << "ResourceBundle initialized twice";
        g_shared_instance_ = new ResourceBundle();

        g_shared_instance_->LoadCommonResources();
        return g_shared_instance_->LoadLocaleResources(pref_locale);
    }

    /* static */
    bool ResourceBundle::ReloadSharedInstance(
        const FilePath& pref_locale)
    {
        DCHECK(g_shared_instance_ != NULL) << "ResourceBundle not initialized";

        g_shared_instance_->UnloadLocaleResources();
        return g_shared_instance_->LoadLocaleResources(pref_locale);
    }

    /* static */
    void ResourceBundle::AddDataPackToSharedInstance(const FilePath& path)
    {
        DCHECK(g_shared_instance_ != NULL) << "ResourceBundle not initialized";
        g_shared_instance_->data_packs_.push_back(new LoadedDataPack(path));
    }

    /* static */
    void ResourceBundle::CleanupSharedInstance()
    {
        if(g_shared_instance_)
        {
            delete g_shared_instance_;
            g_shared_instance_ = NULL;
        }
    }

    /* static */
    ResourceBundle& ResourceBundle::GetSharedInstance()
    {
        // 之前必须调用过InitSharedInstance函数.
        CHECK(g_shared_instance_ != NULL);
        return *g_shared_instance_;
    }

    SkBitmap* ResourceBundle::GetBitmapNamed(int resource_id)
    {
        const SkBitmap* bitmap =
            static_cast<const SkBitmap*>(GetImageNamed(resource_id));
        return const_cast<SkBitmap*>(bitmap);
    }

    gfx::Image& ResourceBundle::GetImageNamed(int resource_id)
    {
        // Check to see if the image is already in the cache.
        {
            base::AutoLock lock_scope(*lock_);
            ImageMap::const_iterator found = images_.find(resource_id);
            if(found != images_.end())
            {
                return *found->second;
            }
        }

        scoped_ptr<SkBitmap> bitmap(LoadBitmap(resources_data_, resource_id));
        if(!bitmap.get())
        {
            bitmap.reset(LoadBitmap(locale_resources_data_, resource_id));
        }

        if(bitmap.get())
        {
            // The load was successful, so cache the image.
            base::AutoLock lock_scope(*lock_);

            // Another thread raced the load and has already cached the image.
            if(images_.count(resource_id))
            {
                return *images_[resource_id];
            }

            gfx::Image* image = new gfx::Image(bitmap.release());
            images_[resource_id] = image;
            return *image;
        }

        // The load failed to retrieve the image; show a debugging red square.
        LOG(WARNING) << "Unable to load image with id " << resource_id;
        NOTREACHED(); // Want to assert in debug mode.
        return *GetEmptyImage();
    }

    // Only Mac and Linux have non-Skia native image types. All other platforms use
    // Skia natively, so just use GetImageNamed().
    gfx::Image& ResourceBundle::GetNativeImageNamed(int resource_id)
    {
        return GetImageNamed(resource_id);
    }

    RefCountedStaticMemory* ResourceBundle::LoadDataResourceBytes(
        int resource_id) const
    {
        RefCountedStaticMemory* bytes = LoadResourceBytes(
            resources_data_, resource_id);

        if(!bytes && locale_resources_data_)
        {
            bytes = LoadResourceBytes(locale_resources_data_, resource_id);
        }

        // 如果主资源以及本地资源中没加载成功, 检查所有的附加数据包.
        for(std::vector<LoadedDataPack*>::const_iterator it=data_packs_.begin();
            !bytes&&it!=data_packs_.end(); ++it)
        {
            bytes = (*it)->GetStaticMemory(resource_id);
        }

        return bytes;
    }

    const gfx::Font& ResourceBundle::GetFont(FontStyle style)
    {
        {
            base::AutoLock lock_scope(*lock_);
            LoadFontsIfNecessary();
        }
        switch(style)
        {
        case BoldFont:
            return *bold_font_;
        case SmallFont:
            return *small_font_;
        case MediumFont:
            return *medium_font_;
        case MediumBoldFont:
            return *medium_bold_font_;
        case LargeFont:
            return *large_font_;
        default:
            return *base_font_;
        }
    }

    void ResourceBundle::ReloadFonts()
    {
        base::AutoLock lock_scope(*lock_);
        base_font_.reset();
        LoadFontsIfNecessary();
    }

    ResourceBundle::ResourceBundle()
        : lock_(new base::Lock),
        resources_data_(NULL),
        locale_resources_data_(NULL) {}

    void ResourceBundle::FreeImages()
    {
        STLDeleteContainerPairSecondPointers(images_.begin(), images_.end());
        images_.clear();
    }

    void ResourceBundle::LoadFontsIfNecessary()
    {
        lock_->AssertAcquired();
        if(!base_font_.get())
        {
            base_font_.reset(new gfx::Font());

            bold_font_.reset(new gfx::Font());
            *bold_font_ = base_font_->DeriveFont(0,
                base_font_->GetStyle()|gfx::Font::BOLD);

            small_font_.reset(new gfx::Font());
            *small_font_ = base_font_->DeriveFont(kSmallFontSizeDelta);

            medium_font_.reset(new gfx::Font());
            *medium_font_ = base_font_->DeriveFont(kMediumFontSizeDelta);

            medium_bold_font_.reset(new gfx::Font());
            *medium_bold_font_ = base_font_->DeriveFont(kMediumFontSizeDelta,
                base_font_->GetStyle()|gfx::Font::BOLD);

            large_font_.reset(new gfx::Font());
            *large_font_ = base_font_->DeriveFont(kLargeFontSizeDelta);
        }
    }

    /* static */
    SkBitmap* ResourceBundle::LoadBitmap(DataHandle data_handle, int resource_id)
    {
        scoped_refptr<RefCountedMemory> memory(
            LoadResourceBytes(data_handle, resource_id));
        if(!memory)
        {
            return NULL;
        }

        SkBitmap bitmap;
        if(!gfx::PNGCodec::Decode(memory->front(), memory->size(), &bitmap))
        {
            NOTREACHED() << "Unable to decode theme image resource " << resource_id;
            return NULL;
        }

        return new SkBitmap(bitmap);
    }

    gfx::Image* ResourceBundle::GetEmptyImage()
    {
        base::AutoLock lock(*lock_);

        static gfx::Image* empty_image = NULL;
        if(!empty_image)
        {
            // The placeholder bitmap is bright red so people notice the problem.
            // This bitmap will be leaked, but this code should never be hit.
            SkBitmap* bitmap = new SkBitmap();
            bitmap->setConfig(SkBitmap::kARGB_8888_Config, 32, 32);
            bitmap->allocPixels();
            bitmap->eraseARGB(255, 255, 0, 0);
            empty_image = new gfx::Image(bitmap);
        }
        return empty_image;
    }

    // LoadedDataPack implementation
    ResourceBundle::LoadedDataPack::LoadedDataPack(const FilePath& path)
        : path_(path)
    {
        // Always preload the data packs so we can maintain constness.
        Load();
    }

    ResourceBundle::LoadedDataPack::~LoadedDataPack() {}

    void ResourceBundle::LoadedDataPack::Load()
    {
        DCHECK(!data_pack_.get());
        data_pack_.reset(new DataPack);
        bool success = data_pack_->Load(path_);
        LOG_IF(ERROR, !success) << "Failed to load " << path_.value()
            << "\nSome features may not be available.";
        if(!success)
        {
            data_pack_.reset();
        }
    }

    bool ResourceBundle::LoadedDataPack::GetStringPiece(
        int resource_id, base::StringPiece* data) const
    {
        return data_pack_->GetStringPiece(static_cast<uint32>(resource_id), data);
    }

    RefCountedStaticMemory* ResourceBundle::LoadedDataPack::GetStaticMemory(
        int resource_id) const
    {
        return data_pack_->GetStaticMemory(resource_id);
    }

    namespace
    {

        // Returns the flags that should be passed to LoadLibraryEx.
        DWORD GetDataDllLoadFlags()
        {
            if(base::win::GetVersion() >= base::win::VERSION_VISTA)
            {
                return LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE |
                    LOAD_LIBRARY_AS_IMAGE_RESOURCE;
            }

            return DONT_RESOLVE_DLL_REFERENCES;
        }

    }

    ResourceBundle::~ResourceBundle()
    {
        FreeImages();
        UnloadLocaleResources();
        STLDeleteContainerPointers(data_packs_.begin(), data_packs_.end());
        resources_data_ = NULL;
    }

    // http://blogs.msdn.com/oldnewthing/archive/2004/10/25/247180.aspx
    extern "C" IMAGE_DOS_HEADER __ImageBase;
    void ResourceBundle::LoadCommonResources()
    {
        // 纯资源DLL, 没有可执行代码.
        DCHECK(NULL == resources_data_) << "common resources already loaded";
        resources_data_ = reinterpret_cast<HMODULE>(&__ImageBase);
    }

    bool ResourceBundle::LoadLocaleResources(
        const FilePath& locale_resource_path)
    {
        DCHECK(NULL == locale_resources_data_) << "locale dll already loaded";

        // 纯资源DLL, 没有可执行代码.
        locale_resources_data_ = LoadLibraryExW(locale_resource_path.value().c_str(),
            NULL, GetDataDllLoadFlags());

        DCHECK(locale_resources_data_ != NULL) << "unable to load generated resources";

        return locale_resources_data_ != NULL;
    }

    void ResourceBundle::UnloadLocaleResources()
    {
        if(locale_resources_data_)
        {
            BOOL rv = FreeLibrary(locale_resources_data_);
            DCHECK(rv);
            locale_resources_data_ = NULL;
        }
    }

    // static
    RefCountedStaticMemory* ResourceBundle::LoadResourceBytes(
        DataHandle module, int resource_id)
    {
        void* data_ptr;
        size_t data_size;
        if(base::GetDataResourceFromModule(module, resource_id, &data_ptr,
            &data_size))
        {
            return new RefCountedStaticMemory(
                reinterpret_cast<const unsigned char*>(data_ptr), data_size);
        }
        else
        {
            return NULL;
        }
    }

    HICON ResourceBundle::LoadThemeIcon(int icon_id)
    {
        HICON icon = ::LoadIcon(resources_data_, MAKEINTRESOURCE(icon_id));
        if(icon == NULL)
        {
            icon = ::LoadIcon(locale_resources_data_, MAKEINTRESOURCE(icon_id));
        }

        return icon;
    }

    base::StringPiece ResourceBundle::GetRawDataResource(int resource_id) const
    {
        void* data_ptr;
        size_t data_size;
        if(base::GetDataResourceFromModule(resources_data_, resource_id, &data_ptr,
            &data_size))
        {
            return base::StringPiece(static_cast<const char*>(data_ptr), data_size);
        }
        else if(locale_resources_data_ && base::GetDataResourceFromModule(
            locale_resources_data_, resource_id, &data_ptr, &data_size))
        {
            return base::StringPiece(static_cast<const char*>(data_ptr), data_size);
        }

        base::StringPiece data;
        for(size_t i=0; i<data_packs_.size(); ++i)
        {
            if(data_packs_[i]->GetStringPiece(resource_id, &data))
            {
                return data;
            }
        }

        return base::StringPiece();
    }

    // Loads and returns a cursor from the current module.
    HCURSOR ResourceBundle::LoadCursor(int cursor_id)
    {
        HCURSOR cursor = ::LoadCursor(resources_data_, MAKEINTRESOURCE(cursor_id));
        if(!cursor)
        {
            cursor = ::LoadCursor(locale_resources_data_, MAKEINTRESOURCE(cursor_id));
        }

        return cursor;
    }

    namespace
    {

#pragma warning(push)
#pragma warning(disable: 4200)
        struct STRINGRESOURCEIMAGE
        {
            WORD nLength;
            WCHAR achString[];
        };
#pragma warning(pop) // C4200

        inline const STRINGRESOURCEIMAGE* _GetStringResourceImage(HINSTANCE hInstance,
            HRSRC hResource, UINT id) throw()
        {
            const STRINGRESOURCEIMAGE* pImage;
            const STRINGRESOURCEIMAGE* pImageEnd;
            ULONG nResourceSize;
            HGLOBAL hGlobal;
            UINT iIndex;

            hGlobal = ::LoadResource(hInstance, hResource);
            if(hGlobal == NULL)
            {
                return NULL;
            }

            pImage = (const STRINGRESOURCEIMAGE*)::LockResource(hGlobal);
            if(pImage == NULL)
            {
                return NULL;
            }

            nResourceSize = ::SizeofResource(hInstance, hResource);
            pImageEnd = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage) + nResourceSize);
            iIndex = id & 0x000f;

            while((iIndex>0) && (pImage<pImageEnd))
            {
                pImage = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage) +
                    (sizeof(STRINGRESOURCEIMAGE) + (pImage->nLength*sizeof(WCHAR))));
                iIndex--;
            }
            if(pImage >= pImageEnd)
            {
                return NULL;
            }
            if(pImage->nLength == 0)
            {
                return NULL;
            }

            return pImage;
        }

        inline const STRINGRESOURCEIMAGE* GetStringResourceImage(HINSTANCE hInstance,
            UINT id) throw()
        {
            HRSRC hResource;

            hResource = ::FindResource(hInstance, MAKEINTRESOURCE(((id>>4)+1)), RT_STRING);
            if(hResource == NULL)
            {
                return NULL;
            }

            return _GetStringResourceImage(hInstance, hResource, id);
        }

    }

    string16 ResourceBundle::GetLocalizedString(int message_id)
    {
        DCHECK(IS_INTRESOURCE(message_id));

        // Get a reference directly to the string resource.
        HINSTANCE hinstance = locale_resources_data_;
        const STRINGRESOURCEIMAGE* image = GetStringResourceImage(hinstance, message_id);
        if(!image)
        {
            // 从当前模块查找字符串(Chrom代码中只是在本地资源模块中查找, 我做了适当修改).
            hinstance = resources_data_;
            image = GetStringResourceImage(hinstance, message_id);
        }
        if(!image)
        {
            base::debug::StackTrace().PrintBacktrace(); // See http://crbug.com/21925.
            NOTREACHED() << "unable to find resource: " << message_id;
            return std::wstring();
        }

        // Copy into a string16 and return.
        return string16(image->achString, image->nLength);
    }

} //namespace ui