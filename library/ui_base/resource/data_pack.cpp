
#include "data_pack.h"

#include "base/file_util.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/metric/histogram.h"
#include "base/string_piece.h"

namespace
{

    static const uint32 kFileFormatVersion = 3;

    // 文件头长度: 版本和资源数量.
    static const size_t kHeaderLength = 2 * sizeof(uint32);

#pragma pack(push,2)
    struct DataPackEntry
    {
        uint16 resource_id;
        uint32 file_offset;

        static int CompareById(const void* void_key, const void* void_entry)
        {
            uint16 key = *reinterpret_cast<const uint16*>(void_key);
            const DataPackEntry* entry =
                reinterpret_cast<const DataPackEntry*>(void_entry);
            if(key < entry->resource_id)
            {
                return -1;
            }
            else if(key > entry->resource_id)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
    };
#pragma pack(pop)

    COMPILE_ASSERT(sizeof(DataPackEntry) == 6, size_of_entry_must_be_six);

    // We're crashing when trying to load a pak file on Windows.  Add some error
    // codes for logging.
    // http://crbug.com/58056
    enum LoadErrors
    {
        INIT_FAILED = 1,
        BAD_VERSION,
        INDEX_TRUNCATED,
        ENTRY_NOT_FOUND,

        LOAD_ERRORS_COUNT,
    };

}

namespace ui
{

    DataPack::DataPack() : resource_count_(0) {}

    DataPack::~DataPack() {}

    bool DataPack::Load(const FilePath& path)
    {
        mmap_.reset(new base::MemoryMappedFile);
        if(!mmap_->Initialize(path))
        {
            DLOG(ERROR) << "Failed to mmap datapack";
            UMA_HISTOGRAM_ENUMERATION("DataPack.Load", INIT_FAILED,
                LOAD_ERRORS_COUNT);
            return false;
        }

        // Sanity check the header of the file.
        if(kHeaderLength > mmap_->length())
        {
            DLOG(ERROR) << "Data pack file corruption: incomplete file header.";
            mmap_.reset();
            return false;
        }

        // 解析文件头. 第一个uint32: 版本; 第二个uint32: 资源数量.
        const uint32* ptr = reinterpret_cast<const uint32*>(mmap_->data());
        uint32 version = ptr[0];
        if(version != kFileFormatVersion)
        {
            LOG(ERROR) << "Bad data pack version: got " << version
                << ", expected " << kFileFormatVersion;
            UMA_HISTOGRAM_ENUMERATION("DataPack.Load", BAD_VERSION,
                LOAD_ERRORS_COUNT);
            mmap_.reset();
            return false;
        }
        resource_count_ = ptr[1];

        // 检查文件的完整性.
        // 1)检查是否有足够的资源数量.
        if(kHeaderLength+resource_count_*sizeof(DataPackEntry) > mmap_->length())
        {
            LOG(ERROR) << "Data pack file corruption: too short for number of "
                "entries specified.";
            UMA_HISTOGRAM_ENUMERATION("DataPack.Load", INDEX_TRUNCATED,
                LOAD_ERRORS_COUNT);
            mmap_.reset();
            return false;
        }
        // 2)验证所有的资源边界.
        for(size_t i=0; i<resource_count_+1; ++i)
        {
            const DataPackEntry* entry = reinterpret_cast<const DataPackEntry*>(
                mmap_->data() + kHeaderLength + (i * sizeof(DataPackEntry)));
            if(entry->file_offset > mmap_->length())
            {
                LOG(ERROR) << "Entry #" << i << " in data pack points off end of file. "
                    << "Was the file corrupted?";
                UMA_HISTOGRAM_ENUMERATION("DataPack.Load", ENTRY_NOT_FOUND,
                    LOAD_ERRORS_COUNT);
                mmap_.reset();
                return false;
            }
        }

        return true;
    }

    bool DataPack::GetStringPiece(uint16 resource_id, base::StringPiece* data) const
    {
        const DataPackEntry* target = reinterpret_cast<const DataPackEntry*>(
            bsearch(&resource_id, mmap_->data()+kHeaderLength, resource_count_,
            sizeof(DataPackEntry), DataPackEntry::CompareById));
        if(!target)
        {
            return false;
        }

        const DataPackEntry* next_entry = target + 1;
        size_t length = next_entry->file_offset - target->file_offset;

        data->set(mmap_->data()+target->file_offset, length);
        return true;
    }

    RefCountedStaticMemory* DataPack::GetStaticMemory(uint16 resource_id) const
    {
        base::StringPiece piece;
        if(!GetStringPiece(resource_id, &piece))
        {
            return NULL;
        }

        return new RefCountedStaticMemory(
            reinterpret_cast<const unsigned char*>(piece.data()), piece.length());
    }

    // static
    bool DataPack::WritePack(const FilePath& path,
        const std::map<uint16, base::StringPiece>& resources)
    {
        FILE* file = base::OpenFile(path, "wb");
        if(!file)
        {
            return false;
        }

        if(fwrite(&kFileFormatVersion, sizeof(kFileFormatVersion), 1, file) != 1)
        {
            LOG(ERROR) << "Failed to write file version";
            base::CloseFile(file);
            return false;
        }

        uint32 entry_count = resources.size();
        if(fwrite(&entry_count, sizeof(entry_count), 1, file) != 1)
        {
            LOG(ERROR) << "Failed to write entry count";
            base::CloseFile(file);
            return false;
        }

        // Each entry is a uint16 + a uint32. We have an extra entry after the last
        // item so we can compute the size of the list item.
        uint32 index_length = (entry_count + 1) * sizeof(DataPackEntry);
        uint32 data_offset = kHeaderLength + index_length;
        for(std::map<uint16, base::StringPiece>::const_iterator it=resources.begin();
            it!=resources.end(); ++it)
        {
            uint16 resource_id = it->first;
            if(fwrite(&resource_id, sizeof(resource_id), 1, file) != 1)
            {
                LOG(ERROR) << "Failed to write id for " << resource_id;
                base::CloseFile(file);
                return false;
            }

            if(fwrite(&data_offset, sizeof(data_offset), 1, file) != 1)
            {
                LOG(ERROR) << "Failed to write offset for " << resource_id;
                base::CloseFile(file);
                return false;
            }

            data_offset += it->second.length();
        }

        // We place an extra entry after the last item that allows us to read the
        // size of the last item.
        uint16 resource_id = 0;
        if(fwrite(&resource_id, sizeof(resource_id), 1, file) != 1)
        {
            LOG(ERROR) << "Failed to write extra resource id.";
            base::CloseFile(file);
            return false;
        }

        if(fwrite(&data_offset, sizeof(data_offset), 1, file) != 1)
        {
            LOG(ERROR) << "Failed to write extra offset.";
            base::CloseFile(file);
            return false;
        }

        for(std::map<uint16, base::StringPiece>::const_iterator it=resources.begin();
            it!=resources.end(); ++it)
        {
            if(fwrite(it->second.data(), it->second.length(), 1, file) != 1)
            {
                LOG(ERROR) << "Failed to write data for " << it->first;
                base::CloseFile(file);
                return false;
            }
        }

        base::CloseFile(file);

        return true;
    }

} //namespace ui