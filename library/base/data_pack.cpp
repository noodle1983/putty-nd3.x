
#include "data_pack.h"

#include "file_util.h"
#include "logging.h"
#include "ref_counted_memory.h"
#include "string_piece.h"

namespace
{

    // 一个字长为4字节.
    static const size_t kWord = 4;

    static const uint32 kFileFormatVersion = 1;
    // 文件头长度: 版本和资源数量.
    static const size_t kHeaderLength = 2 * sizeof(uint32);

    struct DataPackEntry
    {
        uint32 resource_id;
        uint32 file_offset;
        uint32 length;

        static int CompareById(const void* void_key, const void* void_entry)
        {
            uint32 key = *reinterpret_cast<const uint32*>(void_key);
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

    COMPILE_ASSERT(sizeof(DataPackEntry)==12, size_of_header_must_be_twelve);

}

namespace base
{

    DataPack::DataPack() : resource_count_(0) {}

    DataPack::~DataPack() {}

    bool DataPack::Load(const FilePath& path)
    {
        mmap_.reset(new MemoryMappedFile);
        if(!mmap_->Initialize(path))
        {
            DLOG(ERROR) << "Failed to mmap datapack";
            return false;
        }

        // 解析文件头. 第一个uint32: 版本; 第二个uint32: 资源数量.
        const uint32* ptr = reinterpret_cast<const uint32*>(mmap_->data());
        uint32 version = ptr[0];
        if(version != kFileFormatVersion)
        {
            LOG(ERROR) << "Bad data pack version: got " << version
                << ", expected " << kFileFormatVersion;
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
            mmap_.reset();
            return false;
        }
        // 2)验证所有的资源边界.
        for(size_t i=0; i<resource_count_; ++i)
        {
            const DataPackEntry* entry = reinterpret_cast<const DataPackEntry*>(
                mmap_->data() + kHeaderLength + (i*sizeof(DataPackEntry)));
            if(entry->file_offset+entry->length > mmap_->length())
            {
                LOG(ERROR) << "Entry #" << i << " in data pack points off end of "
                    << "file. Was the file corrupted?";
                mmap_.reset();
                return false;
            }
        }

        return true;
    }

    bool DataPack::GetStringPiece(uint32 resource_id, StringPiece* data) const
    {
        DataPackEntry* target = reinterpret_cast<DataPackEntry*>(
            bsearch(&resource_id, mmap_->data()+kHeaderLength, resource_count_,
            sizeof(DataPackEntry), DataPackEntry::CompareById));
        if(!target)
        {
            return false;
        }

        data->set(mmap_->data()+target->file_offset, target->length);
        return true;
    }

    RefCountedStaticMemory* DataPack::GetStaticMemory(uint32 resource_id) const
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
        const std::map<uint32, StringPiece>& resources)
    {
        FILE* file = OpenFile(path, "wb");
        if(!file)
        {
            return false;
        }

        if(fwrite(&kFileFormatVersion, 1, kWord, file) != kWord)
        {
            LOG(ERROR) << "Failed to write file version";
            CloseFile(file);
            return false;
        }

        uint32 entry_count = resources.size();
        if(fwrite(&entry_count, 1, kWord, file) != kWord)
        {
            LOG(ERROR) << "Failed to write entry count";
            CloseFile(file);
            return false;
        }

        // 每个资源都是3个uint32s.
        uint32 index_length = entry_count * 3 * kWord;
        uint32 data_offset = kHeaderLength + index_length;
        for (std::map<uint32, StringPiece>::const_iterator it=resources.begin();
            it!=resources.end(); ++it)
        {
            if(fwrite(&it->first, 1, kWord, file) != kWord)
            {
                LOG(ERROR) << "Failed to write id for " << it->first;
                CloseFile(file);
                return false;
            }

            if(fwrite(&data_offset, 1, kWord, file) != kWord)
            {
                LOG(ERROR) << "Failed to write offset for " << it->first;
                CloseFile(file);
                return false;
            }

            uint32 len = it->second.length();
            if(fwrite(&len, 1, kWord, file) != kWord)
            {
                LOG(ERROR) << "Failed to write length for " << it->first;
                CloseFile(file);
                return false;
            }

            data_offset += len;
        }

        for(std::map<uint32, StringPiece>::const_iterator it=resources.begin();
            it!=resources.end(); ++it)
        {
            if(fwrite(it->second.data(), it->second.length(), 1, file) != 1)
            {
                LOG(ERROR) << "Failed to write data for " << it->first;
                CloseFile(file);
                return false;
            }
        }

        CloseFile(file);

        return true;
    }

} //namespace base