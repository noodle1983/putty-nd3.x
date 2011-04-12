
// DataPack表示以(key, value)为数据的磁盘文件只读视图. 用于存储静态资源,
// 比如字符串和图像.

#ifndef __base_data_pack_h__
#define __base_data_pack_h__

#pragma once

#include <map>

#include "basic_types.h"
#include "memory/scoped_ptr.h"

class FilePath;

namespace base
{

    class MemoryMappedFile;
    class RefCountedStaticMemory;
    class StringPiece;

    class DataPack
    {
    public:
        DataPack();
        ~DataPack();

        // 从|path|加载打包文件, 发生错误时返回false.
        bool Load(const FilePath& path);

        // 通过|resource_id|获取资源, 填充数据到|data|. 数据归DataPack对象所有,
        // 不要修改. 如果没找到资源id, 返回false.
        bool GetStringPiece(uint32 resource_id, StringPiece* data) const;

        // 类似GetStringPiece(), 但是返回内存的指针. 本接口用于图像数据,
        // StringPiece接口一般用于本地字符串.
        RefCountedStaticMemory* GetStaticMemory(uint32 resource_id) const;

        // 将|resources|写入到路径为|path|的打包文件.
        static bool WritePack(const FilePath& path,
            const std::map<uint32, StringPiece>& resources);

    private:
        // 内存映射数据.
        scoped_ptr<MemoryMappedFile> mmap_;

        // 数据中的资源数量.
        size_t resource_count_;

        DISALLOW_COPY_AND_ASSIGN(DataPack);
    };

} //namespace base

#endif //__base_data_pack_h__