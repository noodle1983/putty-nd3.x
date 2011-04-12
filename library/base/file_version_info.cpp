
#include "file_version_info.h"

#include "file_path.h"
#include "logging.h"
#include "path_service.h"
#include "threading/thread_restrictions.h"

#pragma comment(lib, "version.lib")

FileVersionInfo::FileVersionInfo(void* data, int language, int code_page)
: language_(language), code_page_(code_page)
{
    base::ThreadRestrictions::AssertIOAllowed();
    data_.reset((char*)data);
    fixed_file_info_ = NULL;
    UINT size;
    VerQueryValueW(data_.get(), L"\\", (LPVOID*)&fixed_file_info_, &size);
}

FileVersionInfo::~FileVersionInfo()
{
    DCHECK(data_.get());
}

typedef struct
{
    WORD language;
    WORD code_page;
} LanguageAndCodePage;

// static
FileVersionInfo* FileVersionInfo::CreateFileVersionInfoForCurrentModule()
{
    FilePath app_path;
    if(!PathProvider(base::FILE_MODULE, &app_path))
    {
        return NULL;
    }

    return CreateFileVersionInfo(app_path);
}

// static
FileVersionInfo* FileVersionInfo::CreateFileVersionInfo(const FilePath& file_path)
{
    base::ThreadRestrictions::AssertIOAllowed();

    DWORD dummy;
    const wchar_t* path = file_path.value().c_str();
    DWORD length = ::GetFileVersionInfoSize(path, &dummy);
    if(length == 0)
    {
        return NULL;
    }

    void* data = calloc(length, 1);
    if(!data)
    {
        return NULL;
    }

    if(!::GetFileVersionInfo(path, dummy, length, data))
    {
        free(data);
        return NULL;
    }

    LanguageAndCodePage* translate = NULL;
    uint32 page_count;
    BOOL query_result = VerQueryValue(data, L"\\VarFileInfo\\Translation",
        (void**) &translate, &page_count);

    if(query_result && translate)
    {
        return new FileVersionInfo(data, translate->language,
            translate->code_page);

    }
    else
    {
        free(data);
        return NULL;
    }
}

std::wstring FileVersionInfo::company_name()
{
    return GetStringValue(L"CompanyName");
}

std::wstring FileVersionInfo::company_short_name()
{
    return GetStringValue(L"CompanyShortName");
}

std::wstring FileVersionInfo::internal_name()
{
    return GetStringValue(L"InternalName");
}

std::wstring FileVersionInfo::product_name()
{
    return GetStringValue(L"ProductName");
}

std::wstring FileVersionInfo::product_short_name()
{
    return GetStringValue(L"ProductShortName");
}

std::wstring FileVersionInfo::comments()
{
    return GetStringValue(L"Comments");
}

std::wstring FileVersionInfo::legal_copyright()
{
    return GetStringValue(L"LegalCopyright");
}

std::wstring FileVersionInfo::product_version()
{
    return GetStringValue(L"ProductVersion");
}

std::wstring FileVersionInfo::file_description()
{
    return GetStringValue(L"FileDescription");
}

std::wstring FileVersionInfo::legal_trademarks()
{
    return GetStringValue(L"LegalTrademarks");
}

std::wstring FileVersionInfo::private_build()
{
    return GetStringValue(L"PrivateBuild");
}

std::wstring FileVersionInfo::file_version()
{
    return GetStringValue(L"FileVersion");
}

std::wstring FileVersionInfo::original_filename()
{
    return GetStringValue(L"OriginalFilename");
}

std::wstring FileVersionInfo::special_build()
{
    return GetStringValue(L"SpecialBuild");
}

std::wstring FileVersionInfo::last_change()
{
    return GetStringValue(L"LastChange");
}

bool FileVersionInfo::is_official_build()
{
    return GetStringValue(L"Official Build").compare(L"1") == 0;
}

bool FileVersionInfo::GetValue(const wchar_t* name, std::wstring& value_str)
{
    WORD lang_codepage[8];
    int i = 0;
    // Use the language and codepage from the DLL.
    lang_codepage[i++] = language_;
    lang_codepage[i++] = code_page_;
    // Use the default language and codepage from the DLL.
    lang_codepage[i++] = GetUserDefaultLangID();
    lang_codepage[i++] = code_page_;
    // Use the language from the DLL and Latin codepage (most common).
    lang_codepage[i++] = language_;
    lang_codepage[i++] = 1252;
    // Use the default language and Latin codepage (most common).
    lang_codepage[i++] = GetUserDefaultLangID();
    lang_codepage[i++] = 1252;

    i = 0;
    while(i < sizeof(lang_codepage)/sizeof(lang_codepage[0]))
    {
        wchar_t sub_block[MAX_PATH];
        WORD language = lang_codepage[i++];
        WORD code_page = lang_codepage[i++];
        _snwprintf_s(sub_block, MAX_PATH, MAX_PATH,
            L"\\StringFileInfo\\%04x%04x\\%ls", language, code_page, name);
        LPVOID value = NULL;
        unsigned int size;
        BOOL r = VerQueryValueW(data_.get(), sub_block, &value, &size);
        if(r && value)
        {
            value_str.assign(static_cast<wchar_t*>(value));
            return true;
        }
    }
    return false;
}

std::wstring FileVersionInfo::GetStringValue(const wchar_t* name)
{
    std::wstring str;
    if(GetValue(name, str))
    {
        return str;
    }
    else
    {
        return L"";
    }
}