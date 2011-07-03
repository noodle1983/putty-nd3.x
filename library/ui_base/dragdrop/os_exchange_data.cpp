
#include "os_exchange_data.h"

#include "base/pickle.h"

namespace ui
{

    OSExchangeData::OSExchangeData() : provider_(CreateProvider()) {}

    OSExchangeData::OSExchangeData(Provider* provider)
        : provider_(provider) {}

    OSExchangeData::~OSExchangeData() {}

    void OSExchangeData::SetString(const string16& data)
    {
        provider_->SetString(data);
    }

    void OSExchangeData::SetFilename(const FilePath& path)
    {
        provider_->SetFilename(path);
    }

    void OSExchangeData::SetPickledData(CustomFormat format, const Pickle& data)
    {
        provider_->SetPickledData(format, data);
    }

    bool OSExchangeData::GetString(string16* data) const
    {
        return provider_->GetString(data);
    }

    bool OSExchangeData::GetFilename(FilePath* path) const
    {
        return provider_->GetFilename(path);
    }

    bool OSExchangeData::GetPickledData(CustomFormat format, Pickle* data) const
    {
        return provider_->GetPickledData(format, data);
    }

    bool OSExchangeData::HasString() const
    {
        return provider_->HasString();
    }

    bool OSExchangeData::HasFile() const
    {
        return provider_->HasFile();
    }

    bool OSExchangeData::HasCustomFormat(CustomFormat format) const
    {
        return provider_->HasCustomFormat(format);
    }

    bool OSExchangeData::HasAllFormats(int formats,
        const std::set<CustomFormat>& custom_formats) const
    {
        if((formats&STRING)!=0 && !HasString())
        {
            return false;
        }
        if((formats&FILE_CONTENTS)!=0 && !provider_->HasFileContents())
        {
            return false;
        }
        if((formats&FILE_NAME)!=0 && !provider_->HasFile())
        {
            return false;
        }
        for(std::set<CustomFormat>::const_iterator i=custom_formats.begin();
            i!=custom_formats.end(); ++i)
        {
            if(!HasCustomFormat(*i))
            {
                return false;
            }
        }
        return true;
    }

    bool OSExchangeData::HasAnyFormat(int formats,
        const std::set<CustomFormat>& custom_formats) const
    {
        if((formats&STRING) != 0 && HasString())
        {
            return true;
        }
        if((formats&FILE_CONTENTS)!=0 && provider_->HasFileContents())
        {
            return true;
        }
        if((formats&FILE_NAME)!=0 && provider_->HasFile())
        {
            return true;
        }
        for(std::set<CustomFormat>::const_iterator i=custom_formats.begin();
            i!=custom_formats.end(); ++i)
        {
            if(HasCustomFormat(*i))
            {
                return true;
            }
        }
        return false;
    }

    void OSExchangeData::SetFileContents(const FilePath& filename,
        const std::string& file_contents)
    {
        provider_->SetFileContents(filename, file_contents);
    }

    bool OSExchangeData::GetFileContents(FilePath* filename,
        std::string* file_contents) const
    {
        return provider_->GetFileContents(filename, file_contents);
    }

} //namespace ui