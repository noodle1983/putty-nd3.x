
#ifndef __base_registry_h__
#define __base_registry_h__

#pragma once

#include <windows.h>

#include <string>

#include "../basic_types.h"

namespace base
{

    // 读写操作Windows注册表的工具类. 注册表词汇表: "key"类似于文件夹, 里面有
    // "values", 每个value由一些<name, data>对组成, 其中data关联某种数据类型.
    class RegKey
    {
    public:
        RegKey();
        RegKey(HKEY rootkey, const wchar_t* subkey, REGSAM access);
        ~RegKey();

        LONG Create(HKEY rootkey, const wchar_t* subkey, REGSAM access);

        LONG CreateWithDisposition(HKEY rootkey, const wchar_t* subkey,
            DWORD* disposition, REGSAM access);

        LONG Open(HKEY rootkey, const wchar_t* subkey, REGSAM access);

        // 创建子键, 如果存在则打开.
        LONG CreateKey(const wchar_t* name, REGSAM access);

        // 打开子键.
        LONG OpenKey(const wchar_t* name, REGSAM access);

        void Close();

        DWORD ValueCount() const;

        // 第n个value的名字.
        LONG ReadName(int index, std::wstring* name);

        // 如果key合法返回true.
        bool Valid() const { return key_ != NULL; }

        // 删除一个键值包括所有子键值; 使用时小心.
        LONG DeleteKey(const wchar_t* name);

        // 删除键中的一个值.
        LONG DeleteValue(const wchar_t* name);

        bool ValueExists(const wchar_t* name);

        LONG ReadValue(const wchar_t* name, void* data, DWORD* dsize,
            DWORD* dtype) const;
        LONG ReadValue(const wchar_t* name, std::wstring* value) const;
        LONG ReadValueDW(const wchar_t* name, DWORD* value) const;
        LONG ReadInt64(const wchar_t* name, int64* value) const;

        LONG WriteValue(const wchar_t* name, const void* data, DWORD dsize,
            DWORD dtype);
        LONG WriteValue(const wchar_t* name, const wchar_t* value);
        LONG WriteValue(const wchar_t* name, DWORD value);

        // 开始监视键值是否被修改. 键打开的时候必须带上KEY_NOTIFY访问权限.
        LONG StartWatching();

        // 如果没调用StartWatching, 总是返回false. 如果键下面任何东西被修改,
        // 方法返回true. |watch_event_|可能被更新, 函数不能用const.
        bool HasChanged();

        // 如果没手动调用, 析构函数会自动调用. 如果正在监视返回true, 否则false.
        LONG StopWatching();

        inline bool IsWatching() const { return watch_event_ != 0; }
        HANDLE watch_event() const { return watch_event_; }
        HKEY Handle() const { return key_; }

    private:
        HKEY key_; // 当前访问的注册表键.
        HANDLE watch_event_;

        DISALLOW_COPY_AND_ASSIGN(RegKey);
    };

    // 遍历注册表特定键下所有value. 在这个应用中碰巧不需要数据长度
    // 大于MAX_PATH, 但是其它情况下这不一定够.
    class RegistryValueIterator
    {
    public:
        RegistryValueIterator(HKEY root_key, const wchar_t* folder_key);
        ~RegistryValueIterator();

        DWORD ValueCount() const;

        // 如果iterator合法返回true.
        bool Valid() const;

        // 向前递增到下一个value.
        void operator++();

        const wchar_t* Name() const { return name_; }
        const wchar_t* Value() const { return value_; }
        DWORD ValueSize() const { return value_size_; }
        DWORD Type() const { return type_; }

        int Index() const { return index_; }

    private:
        // 读取当前value.
        bool Read();

        // 当前访问的注册表键.
        HKEY key_;

        // 当前访问的value索引.
        int index_;

        // 当前value.
        wchar_t name_[MAX_PATH];
        wchar_t value_[MAX_PATH];
        DWORD value_size_;
        DWORD type_;

        DISALLOW_COPY_AND_ASSIGN(RegistryValueIterator);
    };

    class RegistryKeyIterator
    {
    public:
        RegistryKeyIterator(HKEY root_key, const wchar_t* folder_key);
        ~RegistryKeyIterator();

        DWORD SubkeyCount() const;

        // 如果iterator合法返回true.
        bool Valid() const;

        // 向前递增到下一个键.
        void operator++();

        const wchar_t* Name() const { return name_; }

        int Index() const { return index_; }

    private:
        // 读取当前键.
        bool Read();

        // 当前访问的注册表键.
        HKEY key_;

        // 当前访问的键索引.
        int index_;

        wchar_t name_[MAX_PATH];

        DISALLOW_COPY_AND_ASSIGN(RegistryKeyIterator);
    };

} //namespace base

#endif //__base_registry_h__