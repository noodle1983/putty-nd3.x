
#include "os_exchange_data_provider_win.h"

#include "base/pickle.h"
#include "base/stl_utilinl.h"
#include "base/utf_string_conversions.h"
#include "base/win/scoped_hglobal.h"

#include "ui_base/clipboard/clipboard_util_win.h"

namespace ui
{

    // Creates a new STGMEDIUM object to hold the specified text. The caller
    // owns the resulting object. The "Bytes" version does not NULL terminate, the
    // string version does.
    static STGMEDIUM* GetStorageForBytes(const char* data, size_t bytes);
    static STGMEDIUM* GetStorageForString16(const string16& data);
    static STGMEDIUM* GetStorageForString(const std::string& data);
    // Creates a new STGMEDIUM object to hold a file.
    static STGMEDIUM* GetStorageForFileName(const FilePath& path);
    // Creates a File Descriptor for the creation of a file to the given URL and
    // returns a handle to it.
    static STGMEDIUM* GetStorageForFileDescriptor(const FilePath& path);

    ///////////////////////////////////////////////////////////////////////////////
    // FormatEtcEnumerator

    //
    // This object implements an enumeration interface. The existence of an
    // implementation of this interface is exposed to clients through
    // OSExchangeData's EnumFormatEtc method. Our implementation is nobody's
    // business but our own, so it lives in this file.
    //
    // This Windows API is truly a gem. It wants to be an enumerator but assumes
    // some sort of sequential data (why not just use an array?). See comments
    // throughout.
    class FormatEtcEnumerator : public IEnumFORMATETC
    {
    public:
        FormatEtcEnumerator(DataObjectImpl::StoredData::const_iterator begin,
            DataObjectImpl::StoredData::const_iterator end);
        ~FormatEtcEnumerator();

        // IEnumFORMATETC implementation:
        HRESULT __stdcall Next(ULONG count, FORMATETC* elements_array,
            ULONG* elements_fetched);
        HRESULT __stdcall Skip(ULONG skip_count);
        HRESULT __stdcall Reset();
        HRESULT __stdcall Clone(IEnumFORMATETC** clone);

        // IUnknown implementation:
        HRESULT __stdcall QueryInterface(const IID& iid, void** object);
        ULONG __stdcall AddRef();
        ULONG __stdcall Release();

    private:
        // This can only be called from |CloneFromOther|, since it initializes the
        // contents_ from the other enumerator's contents.
        FormatEtcEnumerator() : ref_count_(0) {}

        // Clone a new FormatEtc from another instance of this enumeration.
        static FormatEtcEnumerator* CloneFromOther(const FormatEtcEnumerator* other);

    private:
        // We are _forced_ to use a vector as our internal data model as Windows'
        // retarded IEnumFORMATETC API assumes a deterministic ordering of elements
        // through methods like Next and Skip. This exposes the underlying data
        // structure to the user. Bah.
        std::vector<FORMATETC*> contents_;

        // The cursor of the active enumeration - an index into |contents_|.
        size_t cursor_;

        LONG ref_count_;

        DISALLOW_COPY_AND_ASSIGN(FormatEtcEnumerator);
    };

    // Safely makes a copy of all of the relevant bits of a FORMATETC object.
    static void CloneFormatEtc(FORMATETC* source, FORMATETC* clone)
    {
        *clone = *source;
        if(source->ptd)
        {
            source->ptd = static_cast<DVTARGETDEVICE*>(
                CoTaskMemAlloc(sizeof(DVTARGETDEVICE)));
            *(clone->ptd) = *(source->ptd);
        }
    }

    FormatEtcEnumerator::FormatEtcEnumerator(
        DataObjectImpl::StoredData::const_iterator start,
        DataObjectImpl::StoredData::const_iterator end)
        : ref_count_(0), cursor_(0)
    {
        // Copy FORMATETC data from our source into ourselves.
        while(start != end)
        {
            FORMATETC* format_etc = new FORMATETC;
            CloneFormatEtc(&(*start)->format_etc, format_etc);
            contents_.push_back(format_etc);
            ++start;
        }
    }

    FormatEtcEnumerator::~FormatEtcEnumerator()
    {
        STLDeleteContainerPointers(contents_.begin(), contents_.end());
    }

    STDMETHODIMP FormatEtcEnumerator::Next(ULONG count, FORMATETC* elements_array,
        ULONG* elements_fetched)
    {
        // MSDN says |elements_fetched| is allowed to be NULL if count is 1.
        if(!elements_fetched)
        {
            DCHECK_EQ(count, 1ul);
        }

        // This method copies count elements into |elements_array|.
        ULONG index = 0;
        while(cursor_<contents_.size() && index<count)
        {
            CloneFormatEtc(contents_[cursor_], &elements_array[index]);
            ++cursor_;
            ++index;
        }
        // The out param is for how many we actually copied.
        if(elements_fetched)
        {
            *elements_fetched = index;
        }

        // If the two don't agree, then we fail.
        return index==count ? S_OK : S_FALSE;
    }

    STDMETHODIMP FormatEtcEnumerator::Skip(ULONG skip_count)
    {
        cursor_ += skip_count;
        // MSDN implies it's OK to leave the enumerator trashed.
        // "Whatever you say, boss"
        return cursor_<=contents_.size() ? S_OK : S_FALSE;
    }

    STDMETHODIMP FormatEtcEnumerator::Reset()
    {
        cursor_ = 0;
        return S_OK;
    }

    STDMETHODIMP FormatEtcEnumerator::Clone(IEnumFORMATETC** clone)
    {
        // Clone the current enumerator in its exact state, including cursor.
        FormatEtcEnumerator* e = CloneFromOther(this);
        e->AddRef();
        *clone = e;
        return S_OK;
    }

    STDMETHODIMP FormatEtcEnumerator::QueryInterface(const IID& iid,
        void** object)
    {
        *object = NULL;
        if(IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IEnumFORMATETC))
        {
            *object = this;
        }
        else
        {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG FormatEtcEnumerator::AddRef()
    {
        return InterlockedIncrement(&ref_count_);
    }

    ULONG FormatEtcEnumerator::Release()
    {
        if(InterlockedDecrement(&ref_count_) == 0)
        {
            ULONG copied_refcnt = ref_count_;
            delete this;
            return copied_refcnt;
        }
        return ref_count_;
    }

    // static
    FormatEtcEnumerator* FormatEtcEnumerator::CloneFromOther(
        const FormatEtcEnumerator* other)
    {
        FormatEtcEnumerator* e = new FormatEtcEnumerator;
        // Copy FORMATETC data from our source into ourselves.
        std::vector<FORMATETC*>::const_iterator start = other->contents_.begin();
        while(start != other->contents_.end())
        {
            FORMATETC* format_etc = new FORMATETC;
            CloneFormatEtc(*start, format_etc);
            e->contents_.push_back(format_etc);
            ++start;
        }
        // Carry over
        e->cursor_ = other->cursor_;
        return e;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // OSExchangeDataProviderWin, public:

    // static
    DataObjectImpl* OSExchangeDataProviderWin::GetDataObjectImpl(
        const OSExchangeData& data)
    {
        return static_cast<const OSExchangeDataProviderWin*>(&data.provider())->
            data_.get();
    }

    // static
    IDataObject* OSExchangeDataProviderWin::GetIDataObject(
        const OSExchangeData& data)
    {
        return static_cast<const OSExchangeDataProviderWin*>(&data.provider())->
            data_object();
    }

    OSExchangeDataProviderWin::OSExchangeDataProviderWin(IDataObject* source)
        : data_(new DataObjectImpl()), source_object_(source) {}

    OSExchangeDataProviderWin::OSExchangeDataProviderWin()
        : data_(new DataObjectImpl()), source_object_(data_.get()) {}

    OSExchangeDataProviderWin::~OSExchangeDataProviderWin() {}

    void OSExchangeDataProviderWin::SetString(const string16& data)
    {
        STGMEDIUM* storage = GetStorageForString16(data);
        data_->contents_.push_back(
            new DataObjectImpl::StoredDataInfo(CF_UNICODETEXT, storage));

        // Also add plain text.
        storage = GetStorageForString(UTF16ToUTF8(data));
        data_->contents_.push_back(
            new DataObjectImpl::StoredDataInfo(CF_TEXT, storage));
    }

    void OSExchangeDataProviderWin::SetFilename(const FilePath& path)
    {
        STGMEDIUM* storage = GetStorageForFileName(path);
        DataObjectImpl::StoredDataInfo* info =
            new DataObjectImpl::StoredDataInfo(CF_HDROP, storage);
        data_->contents_.push_back(info);
    }

    void OSExchangeDataProviderWin::SetPickledData(CLIPFORMAT format,
        const Pickle& data)
    {
        STGMEDIUM* storage = GetStorageForString(
            std::string(static_cast<const char *>(data.data()),
            static_cast<size_t>(data.size())));
        data_->contents_.push_back(
            new DataObjectImpl::StoredDataInfo(format, storage));
    }

    void OSExchangeDataProviderWin::SetFileContents(
        const FilePath& filename,
        const std::string& file_contents)
    {
        // Add CFSTR_FILEDESCRIPTOR
        STGMEDIUM* storage = GetStorageForFileDescriptor(filename);
        data_->contents_.push_back(new DataObjectImpl::StoredDataInfo(
            ClipboardUtil::GetFileDescriptorFormat()->cfFormat, storage));

        // Add CFSTR_FILECONTENTS
        storage = GetStorageForBytes(file_contents.data(), file_contents.length());
        data_->contents_.push_back(new DataObjectImpl::StoredDataInfo(
            ClipboardUtil::GetFileContentFormatZero(), storage));
    }

    bool OSExchangeDataProviderWin::GetString(string16* data) const
    {
        return ClipboardUtil::GetPlainText(source_object_, data);
    }

    bool OSExchangeDataProviderWin::GetFilename(FilePath* path) const
    {
        std::vector<string16> filenames;
        bool success = ClipboardUtil::GetFilenames(source_object_, &filenames);
        if(success)
        {
            *path = FilePath(filenames[0]);
        }
        return success;
    }

    bool OSExchangeDataProviderWin::GetPickledData(CLIPFORMAT format,
        Pickle* data) const
    {
        DCHECK(data);
        FORMATETC format_etc =
        {
            format, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
        };
        bool success = false;
        STGMEDIUM medium;
        if(SUCCEEDED(source_object_->GetData(&format_etc, &medium)))
        {
            if(medium.tymed & TYMED_HGLOBAL)
            {
                base::win::ScopedHGlobal<char> c_data(medium.hGlobal);
                DCHECK_GT(c_data.Size(), 0u);
                // Need to subtract 1 as SetPickledData adds an extra byte to the end.
                *data = Pickle(c_data.get(), static_cast<int>(c_data.Size()-1));
                success = true;
            }
            ReleaseStgMedium(&medium);
        }
        return success;
    }

    bool OSExchangeDataProviderWin::GetFileContents(
        FilePath* filename,
        std::string* file_contents) const
    {
        string16 filename_str;
        if(!ClipboardUtil::GetFileContents(source_object_, &filename_str,
            file_contents))
        {
            return false;
        }
        *filename = FilePath(filename_str);
        return true;
    }

    bool OSExchangeDataProviderWin::HasString() const
    {
        return ClipboardUtil::HasPlainText(source_object_);
    }

    bool OSExchangeDataProviderWin::HasFile() const
    {
        return ClipboardUtil::HasFilenames(source_object_);
    }

    bool OSExchangeDataProviderWin::HasFileContents() const
    {
        return ClipboardUtil::HasFileContents(source_object_);
    }

    bool OSExchangeDataProviderWin::HasCustomFormat(CLIPFORMAT format) const
    {
        FORMATETC format_etc =
        {
            format, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
        };
        return (source_object_->QueryGetData(&format_etc) == S_OK);
    }


    ///////////////////////////////////////////////////////////////////////////////
    // DataObjectImpl, IDataObject implementation:

    // The following function, DuplicateMedium, is derived from WCDataObject.cpp
    // in the WebKit source code. This is the license information for the file:
    /*
    * Copyright (C) 2007 Apple Inc.  All rights reserved.
    *
    * Redistribution and use in source and binary forms, with or without
    * modification, are permitted provided that the following conditions
    * are met:
    * 1. Redistributions of source code must retain the above copyright
    *    notice, this list of conditions and the following disclaimer.
    * 2. Redistributions in binary form must reproduce the above copyright
    *    notice, this list of conditions and the following disclaimer in the
    *    documentation and/or other materials provided with the distribution.
    *
    * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
    * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
    * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    */
    static void DuplicateMedium(CLIPFORMAT source_clipformat,
        STGMEDIUM* source,
        STGMEDIUM* destination)
    {
        switch(source->tymed)
        {
        case TYMED_HGLOBAL:
            destination->hGlobal =
                static_cast<HGLOBAL>(OleDuplicateData(
                source->hGlobal, source_clipformat, 0));
            break;
        case TYMED_MFPICT:
            destination->hMetaFilePict =
                static_cast<HMETAFILEPICT>(OleDuplicateData(
                source->hMetaFilePict, source_clipformat, 0));
            break;
        case TYMED_GDI:
            destination->hBitmap =
                static_cast<HBITMAP>(OleDuplicateData(
                source->hBitmap, source_clipformat, 0));
            break;
        case TYMED_ENHMF:
            destination->hEnhMetaFile =
                static_cast<HENHMETAFILE>(OleDuplicateData(
                source->hEnhMetaFile, source_clipformat, 0));
            break;
        case TYMED_FILE:
            destination->lpszFileName =
                static_cast<LPOLESTR>(OleDuplicateData(
                source->lpszFileName, source_clipformat, 0));
            break;
        case TYMED_ISTREAM:
            destination->pstm = source->pstm;
            destination->pstm->AddRef();
            break;
        case TYMED_ISTORAGE:
            destination->pstg = source->pstg;
            destination->pstg->AddRef();
            break;
        }

        destination->tymed = source->tymed;
        destination->pUnkForRelease = source->pUnkForRelease;
        if(destination->pUnkForRelease)
        {
            destination->pUnkForRelease->AddRef();
        }
    }

    DataObjectImpl::DataObjectImpl() {}

    DataObjectImpl::~DataObjectImpl()
    {
        STLDeleteContainerPointers(contents_.begin(), contents_.end());
    }

    void DataObjectImpl::RemoveData(const FORMATETC& format)
    {
        if(format.ptd)
        {
            return; // Don't attempt to compare target devices.
        }

        for(StoredData::iterator i=contents_.begin(); i!=contents_.end(); ++i)
        {
            if(!(*i)->format_etc.ptd &&
                format.cfFormat==(*i)->format_etc.cfFormat &&
                format.dwAspect==(*i)->format_etc.dwAspect &&
                format.lindex==(*i)->format_etc.lindex &&
                format.tymed==(*i)->format_etc.tymed)
            {
                delete *i;
                contents_.erase(i);
                return;
            }
        }
    }

    HRESULT DataObjectImpl::GetData(FORMATETC* format_etc, STGMEDIUM* medium)
    {
        StoredData::iterator iter = contents_.begin();
        while(iter != contents_.end())
        {
            if((*iter)->format_etc.cfFormat==format_etc->cfFormat &&
                (*iter)->format_etc.lindex==format_etc->lindex &&
                ((*iter)->format_etc.tymed&format_etc->tymed))
            {
                // If medium is NULL, delay-rendering will be used.
                if((*iter)->medium)
                {
                    DuplicateMedium((*iter)->format_etc.cfFormat, (*iter)->medium, medium);
                }
                else
                {
                    // Check if the left button is down.
                    bool is_left_button_down = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;

                    bool wait_for_data = false;
                    if((*iter)->in_delay_rendering)
                    {
                        // Make sure the left button is up. Sometimes the drop target, like
                        // Shell, might be too aggresive in calling GetData when the left
                        // button is not released.
                        if(is_left_button_down)
                        {
                            return DV_E_FORMATETC;
                        }

                        wait_for_data = true;
                    }
                    else
                    {
                        // If the left button is up and the target has not requested the data
                        // yet, it probably means that the target does not support delay-
                        // rendering. So instead, we wait for the data.
                        if(is_left_button_down)
                        {
                            (*iter)->in_delay_rendering = true;
                            memset(medium, 0, sizeof(STGMEDIUM));
                        }
                        else
                        {
                            wait_for_data = true;
                        }
                    }

                    if(!wait_for_data)
                    {
                        return DV_E_FORMATETC;
                    }

                    // The stored data should have been updated with the final version.
                    // So we just need to call this function again to retrieve it.
                    return GetData(format_etc, medium);
                }
                return S_OK;
            }
            ++iter;
        }

        return DV_E_FORMATETC;
    }

    HRESULT DataObjectImpl::GetDataHere(FORMATETC* format_etc,
        STGMEDIUM* medium)
    {
        return DATA_E_FORMATETC;
    }

    HRESULT DataObjectImpl::QueryGetData(FORMATETC* format_etc)
    {
        StoredData::const_iterator iter = contents_.begin();
        while(iter != contents_.end())
        {
            if((*iter)->format_etc.cfFormat == format_etc->cfFormat)
            {
                return S_OK;
            }
            ++iter;
        }
        return DV_E_FORMATETC;
    }

    HRESULT DataObjectImpl::GetCanonicalFormatEtc(
        FORMATETC* format_etc, FORMATETC* result)
    {
        format_etc->ptd = NULL;
        return E_NOTIMPL;
    }

    HRESULT DataObjectImpl::SetData(FORMATETC* format_etc, STGMEDIUM* medium,
        BOOL should_release)
    {
        RemoveData(*format_etc);

        STGMEDIUM* local_medium = new STGMEDIUM;
        if(should_release)
        {
            *local_medium = *medium;
        }
        else
        {
            DuplicateMedium(format_etc->cfFormat, medium, local_medium);
        }

        DataObjectImpl::StoredDataInfo* info =
            new DataObjectImpl::StoredDataInfo(format_etc->cfFormat, local_medium);
        info->medium->tymed = format_etc->tymed;
        info->owns_medium = !!should_release;
        // Make newly added data appear first.
        contents_.insert(contents_.begin(), info);

        return S_OK;
    }

    HRESULT DataObjectImpl::EnumFormatEtc(DWORD direction,
        IEnumFORMATETC** enumerator)
    {
        if(direction == DATADIR_GET)
        {
            FormatEtcEnumerator* e = new FormatEtcEnumerator(
                contents_.begin(), contents_.end());
            e->AddRef();
            *enumerator = e;
            return S_OK;
        }
        return E_NOTIMPL;
    }

    HRESULT DataObjectImpl::DAdvise(FORMATETC* format_etc, DWORD advf,
        IAdviseSink* sink, DWORD* connection)
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }

    HRESULT DataObjectImpl::DUnadvise(DWORD connection)
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }

    HRESULT DataObjectImpl::EnumDAdvise(IEnumSTATDATA** enumerator)
    {
        return OLE_E_ADVISENOTSUPPORTED;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // DataObjectImpl, IUnknown implementation:

    HRESULT DataObjectImpl::QueryInterface(const IID& iid, void** object)
    {
        if(!object)
        {
            return E_POINTER;
        }
        if(IsEqualIID(iid, IID_IDataObject) || IsEqualIID(iid, IID_IUnknown))
        {
            *object = static_cast<IDataObject*>(this);
        }
        else
        {
            *object = NULL;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG DataObjectImpl::AddRef()
    {
        return 0;
    }

    ULONG DataObjectImpl::Release()
    {
        return 0;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // DataObjectImpl, private:

    static STGMEDIUM* GetStorageForBytes(const char* data, size_t bytes)
    {
        HANDLE handle = GlobalAlloc(GPTR, static_cast<int>(bytes));
        base::win::ScopedHGlobal<char> scoped(handle);
        size_t allocated = static_cast<size_t>(GlobalSize(handle));
        memcpy(scoped.get(), data, allocated);

        STGMEDIUM* storage = new STGMEDIUM;
        storage->hGlobal = handle;
        storage->tymed = TYMED_HGLOBAL;
        storage->pUnkForRelease = NULL;
        return storage;
    }

    template<class T>
    static HGLOBAL CopyStringToGlobalHandle(const T& payload)
    {
        int bytes = static_cast<int>(payload.size() + 1) * sizeof(T::value_type);
        HANDLE handle = GlobalAlloc(GPTR, bytes);
        void* data = GlobalLock(handle);
        size_t allocated = static_cast<size_t>(GlobalSize(handle));
        memcpy(data, payload.c_str(), allocated);
        static_cast<T::value_type*>(data)[payload.size()] = '\0';
        GlobalUnlock(handle);
        return handle;
    }

    static STGMEDIUM* GetStorageForString16(const string16& data)
    {
        STGMEDIUM* storage = new STGMEDIUM;
        storage->hGlobal = CopyStringToGlobalHandle<string16>(data);
        storage->tymed = TYMED_HGLOBAL;
        storage->pUnkForRelease = NULL;
        return storage;
    }

    static STGMEDIUM* GetStorageForString(const std::string& data)
    {
        STGMEDIUM* storage = new STGMEDIUM;
        storage->hGlobal = CopyStringToGlobalHandle<std::string>(data);
        storage->tymed = TYMED_HGLOBAL;
        storage->pUnkForRelease = NULL;
        return storage;
    }

    static STGMEDIUM* GetStorageForFileName(const FilePath& path)
    {
        const size_t kDropSize = sizeof(DROPFILES);
        const size_t kTotalBytes =
            kDropSize + (path.value().length() + 2) * sizeof(wchar_t);
        HANDLE hdata = GlobalAlloc(GMEM_MOVEABLE, kTotalBytes);

        base::win::ScopedHGlobal<DROPFILES> locked_mem(hdata);
        DROPFILES* drop_files = locked_mem.get();
        drop_files->pFiles = sizeof(DROPFILES);
        drop_files->fWide = TRUE;
        wchar_t* data = reinterpret_cast<wchar_t*>(
            reinterpret_cast<BYTE*>(drop_files) + kDropSize);
        const size_t copy_size = (path.value().length() + 1) * sizeof(wchar_t);
        memcpy(data, path.value().c_str(), copy_size);
        data[path.value().length() + 1] = L'\0'; // Á½¸öNULL.

        STGMEDIUM* storage = new STGMEDIUM;
        storage->tymed = TYMED_HGLOBAL;
        storage->hGlobal = hdata;
        storage->pUnkForRelease = NULL;
        return storage;
    }

    static STGMEDIUM* GetStorageForFileDescriptor(const FilePath& path)
    {
        string16 file_name = path.value();
        DCHECK(!file_name.empty());
        HANDLE hdata = GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTOR));
        base::win::ScopedHGlobal<FILEGROUPDESCRIPTOR> locked_mem(hdata);

        FILEGROUPDESCRIPTOR* descriptor = locked_mem.get();
        descriptor->cItems = 1;
        descriptor->fgd[0].dwFlags = FD_LINKUI;
        wcsncpy_s(descriptor->fgd[0].cFileName,
            MAX_PATH,
            file_name.c_str(),
            std::min(file_name.size(), MAX_PATH - 1u));

        STGMEDIUM* storage = new STGMEDIUM;
        storage->tymed = TYMED_HGLOBAL;
        storage->hGlobal = hdata;
        storage->pUnkForRelease = NULL;
        return storage;
    }


    ///////////////////////////////////////////////////////////////////////////////
    // OSExchangeData, public:

    // static
    OSExchangeData::Provider* OSExchangeData::CreateProvider()
    {
        return new OSExchangeDataProviderWin();
    }

    // static
    OSExchangeData::CustomFormat OSExchangeData::RegisterCustomFormat(
        const std::string& type)
    {
        return RegisterClipboardFormat(ASCIIToWide(type).c_str());
    }

} //namespace ui