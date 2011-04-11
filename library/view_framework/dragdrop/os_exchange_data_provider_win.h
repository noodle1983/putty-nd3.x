
#ifndef __view_framework_os_exchange_data_provider_win_h__
#define __view_framework_os_exchange_data_provider_win_h__

#pragma once

#include "base/win/scoped_comptr.h"

#include "os_exchange_data.h"

class DataObjectImpl : public IDataObject
{
public:
    DataObjectImpl();

    // IDataObject implementation:
    HRESULT __stdcall GetData(FORMATETC* format_etc, STGMEDIUM* medium);
    HRESULT __stdcall GetDataHere(FORMATETC* format_etc, STGMEDIUM* medium);
    HRESULT __stdcall QueryGetData(FORMATETC* format_etc);
    HRESULT __stdcall GetCanonicalFormatEtc(
        FORMATETC* format_etc, FORMATETC* result);
    HRESULT __stdcall SetData(
        FORMATETC* format_etc, STGMEDIUM* medium, BOOL should_release);
    HRESULT __stdcall EnumFormatEtc(
        DWORD direction, IEnumFORMATETC** enumerator);
    HRESULT __stdcall DAdvise(FORMATETC* format_etc, DWORD advf,
        IAdviseSink* sink, DWORD* connection);
    HRESULT __stdcall DUnadvise(DWORD connection);
    HRESULT __stdcall EnumDAdvise(IEnumSTATDATA** enumerator);

    // IUnknown implementation:
    HRESULT __stdcall QueryInterface(const IID& iid, void** object);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

private:
    // FormatEtcEnumerator only likes us for our StoredDataMap typedef.
    friend class FormatEtcEnumerator;
    friend class OSExchangeDataProviderWin;

    virtual ~DataObjectImpl();

    // Our internal representation of stored data & type info.
    struct StoredDataInfo
    {
        FORMATETC format_etc;
        STGMEDIUM* medium;
        bool owns_medium;
        bool in_delay_rendering;

        StoredDataInfo(CLIPFORMAT cf, STGMEDIUM* medium)
            : medium(medium),
            owns_medium(true),
            in_delay_rendering(false)
        {
            format_etc.cfFormat = cf;
            format_etc.dwAspect = DVASPECT_CONTENT;
            format_etc.lindex = -1;
            format_etc.ptd = NULL;
            format_etc.tymed = medium ? medium->tymed : TYMED_HGLOBAL;
        }

        StoredDataInfo(FORMATETC* format_etc, STGMEDIUM* medium)
            : format_etc(*format_etc),
            medium(medium),
            owns_medium(true),
            in_delay_rendering(false) {}

        ~StoredDataInfo()
        {
            if(owns_medium)
            {
                ReleaseStgMedium(medium);
                delete medium;
            }
        }
    };

    typedef std::vector<StoredDataInfo*> StoredData;
    StoredData contents_;

    base::ScopedComPtr<IDataObject> source_object_;
};

class OSExchangeDataProviderWin : public OSExchangeData::Provider
{
public:
    static DataObjectImpl* GetDataObjectImpl(const OSExchangeData& data);
    static IDataObject* GetIDataObject(const OSExchangeData& data);

    explicit OSExchangeDataProviderWin(IDataObject* source);
    OSExchangeDataProviderWin();

    virtual ~OSExchangeDataProviderWin();

    IDataObject* data_object() const { return data_.get(); }

    // OSExchangeData::Provider methods.
    virtual void SetString(const string16& data);
    virtual void SetFilename(const FilePath& path);
    virtual void SetPickledData(OSExchangeData::CustomFormat format,
        const Pickle& data);
    virtual void SetFileContents(const FilePath& filename,
        const std::string& file_contents);

    virtual bool GetString(string16* data) const;
    virtual bool GetFilename(FilePath* path) const;
    virtual bool GetPickledData(OSExchangeData::CustomFormat format,
        Pickle* data) const;
    virtual bool GetFileContents(FilePath* filename,
        std::string* file_contents) const;
    virtual bool HasString() const;
    virtual bool HasFile() const;
    virtual bool HasFileContents() const;
    virtual bool HasCustomFormat(OSExchangeData::CustomFormat format) const;

private:
    scoped_refptr<DataObjectImpl> data_;
    base::ScopedComPtr<IDataObject> source_object_;

    DISALLOW_COPY_AND_ASSIGN(OSExchangeDataProviderWin);
};

#endif //__view_framework_os_exchange_data_provider_win_h__