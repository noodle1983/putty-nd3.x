
#ifndef __view_framework_os_exchange_data_h__
#define __view_framework_os_exchange_data_h__

#pragma once

#include <set>

#include <objidl.h>

#include "base/basic_types.h"
#include "base/file_path.h"
#include "base/scoped_ptr.h"
#include "base/string16.h"

class Pickle;

///////////////////////////////////////////////////////////////////////////////
//
// OSExchangeData
//  An object that holds interchange data to be sent out to OS services like
//  clipboard, drag and drop, etc. This object exposes an API that clients can
//  use to specify raw data and its high level type. This object takes care of
//  translating that into something the OS can understand.
//
///////////////////////////////////////////////////////////////////////////////
class OSExchangeData
{
public:
    // CustomFormats are used for non-standard data types. For example, bookmark
    // nodes are written using a CustomFormat.
    typedef CLIPFORMAT CustomFormat;

    // Enumeration of the known formats.
    enum Format
    {
        STRING         = 1 << 0,
        FILE_NAME      = 1 << 2,
        PICKLED_DATA   = 1 << 3,
        FILE_CONTENTS  = 1 << 4,
    };


    // Provider defines the platform specific part of OSExchangeData that
    // interacts with the native system.
    class Provider
    {
    public:
        Provider() {}
        virtual ~Provider() {}

        virtual void SetString(const string16& data) = 0;
        virtual void SetFilename(const FilePath& path) = 0;
        virtual void SetPickledData(CustomFormat format, const Pickle& data) = 0;

        virtual bool GetString(string16* data) const = 0;
        virtual bool GetFilename(FilePath* path) const = 0;
        virtual bool GetPickledData(CustomFormat format, Pickle* data) const = 0;

        virtual bool HasString() const = 0;
        virtual bool HasFile() const = 0;
        virtual bool HasCustomFormat(
            OSExchangeData::CustomFormat format) const = 0;

        virtual void SetFileContents(const FilePath& filename,
            const std::string& file_contents) = 0;
        virtual bool GetFileContents(FilePath* filename,
            std::string* file_contents) const = 0;
        virtual bool HasFileContents() const = 0;
    };

    OSExchangeData();
    // Creates an OSExchangeData with the specified provider. OSExchangeData
    // takes ownership of the supplied provider.
    explicit OSExchangeData(Provider* provider);

    ~OSExchangeData();

    // Registers the specific string as a possible format for data.
    static CustomFormat RegisterCustomFormat(const std::string& type);

    // Returns the Provider, which actually stores and manages the data.
    const Provider& provider() const { return *provider_; }
    Provider& provider() { return *provider_; }

    // These functions add data to the OSExchangeData object of various Chrome
    // types. The OSExchangeData object takes care of translating the data into
    // a format suitable for exchange with the OS.
    // NOTE WELL: Typically, a data object like this will contain only one of the
    //            following types of data. In cases where more data is held, the
    //            order in which these functions are called is _important_!
    //       ---> The order types are added to an OSExchangeData object controls
    //            the order of enumeration in our IEnumFORMATETC implementation!
    //            This comes into play when selecting the best (most preferable)
    //            data type for insertion into a DropTarget.
    void SetString(const string16& data);
    // A full path to a file.
    void SetFilename(const FilePath& path);
    // Adds pickled data of the specified format.
    void SetPickledData(CustomFormat format, const Pickle& data);

    // These functions retrieve data of the specified type. If data exists, the
    // functions return and the result is in the out parameter. If the data does
    // not exist, the out parameter is not touched. The out parameter cannot be
    // NULL.
    bool GetString(string16* data) const;
    // Return the path of a file, if available.
    bool GetFilename(FilePath* path) const;
    bool GetPickledData(CustomFormat format, Pickle* data) const;

    // Test whether or not data of certain types is present, without actually
    // returning anything.
    bool HasString() const;
    bool HasFile() const;
    bool HasCustomFormat(CustomFormat format) const;

    // Returns true if this OSExchangeData has data for ALL the formats in
    // |formats| and ALL the custom formats in |custom_formats|.
    bool HasAllFormats(int formats,
        const std::set<CustomFormat>& custom_formats) const;

    // Returns true if this OSExchangeData has data in any of the formats in
    // |formats| or any custom format in |custom_formats|.
    bool HasAnyFormat(int formats,
        const std::set<CustomFormat>& custom_formats) const;

    // Adds the bytes of a file (CFSTR_FILECONTENTS and CFSTR_FILEDESCRIPTOR).
    void SetFileContents(const FilePath& filename,
        const std::string& file_contents);
    bool GetFileContents(FilePath* filename,
        std::string* file_contents) const;

private:
    // Creates the platform specific Provider.
    static Provider* CreateProvider();

    // Provides the actual data.
    scoped_ptr<Provider> provider_;

    DISALLOW_COPY_AND_ASSIGN(OSExchangeData);
};

#endif //__view_framework_os_exchange_data_h__