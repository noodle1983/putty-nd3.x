
// DataPack表示以(key, value)为数据的磁盘文件只读视图. 用于存储静态资源,
// 比如字符串和图像.

#ifndef __ui_base_data_pack_h__
#define __ui_base_data_pack_h__

#pragma once

#include <map>

#include "base/basic_types.h"
#include "base/memory/scoped_ptr.h"

class FilePath;
class RefCountedStaticMemory;

namespace base
{
    class MemoryMappedFile;
    class StringPiece;
}

namespace ui
{

    class DataPack
    {
    public:
        DataPack();
        ~DataPack();

        // 从|path|加载打包文件, 发生错误时返回false.
        bool Load(const FilePath& path);

        // 通过|resource_id|获取资源, 填充数据到|data|. 数据归DataPack对象所有,
        // 不要修改. 如果没找到资源id, 返回false.
        bool GetStringPiece(uint16 resource_id, base::StringPiece* data) const;

        // 类似GetStringPiece(), 但是返回内存的指针. 本接口用于图像数据,
        // StringPiece接口一般用于本地字符串.
        RefCountedStaticMemory* GetStaticMemory(uint16 resource_id) const;

        // 将|resources|写入到路径为|path|的打包文件.
        static bool WritePack(const FilePath& path,
            const std::map<uint16, base::StringPiece>& resources);

    private:
        // 内存映射数据.
        scoped_ptr<base::MemoryMappedFile> mmap_;

        // 数据中的资源数量.
        size_t resource_count_;

        DISALLOW_COPY_AND_ASSIGN(DataPack);
    };

} //namespace ui

#endif //__ui_base_data_pack_h__