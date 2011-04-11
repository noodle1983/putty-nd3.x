
#ifndef __base_file_util_h__
#define __base_file_util_h__

#pragma once

#include <stack>

#include "file_path.h"
#include "platform_file.h"

namespace base
{

    //-----------------------------------------------------------------------------
    // 牵扯到文件系统访问和修改的函数:

    // 返回path目录下在时间|comparison_time|或者更晚创建的文件总数.
    // 不计".."或者".", 目录不递归查找.
    int CountFilesCreatedAfter(const FilePath& path,
        const base::Time& comparison_time);

    // 返回|root_path|下全部文件的总字节数. 如果|root_path|不存在返回0.
    //
    // 函数的实现使用了FileEnumerator类, 所以任何平台下都不会很快.
    int64 ComputeDirectorySize(const FilePath& root_path);

    // 返回|directory|下(不递归)匹配模式|pattern|的全部文件的总字节数.
    // 如果|directory|不存在返回0.
    //
    // 函数的实现使用了FileEnumerator类, 所以任何平台下都不会很快.
    int64 ComputeFilesSize(const FilePath& directory,
        const std::wstring& pattern);

    // 删除路径, 可以是文件或者目录. 如果是目录, 当recursive为true时,
    // 会删除目录所有内容包含子目录, 否则移除目录(空目录).
    // 成功返回true, 否则返回false.
    //
    // 警告: 使用recursive==true等价于"rm -rf", 需要小心.
    bool Delete(const FilePath& path, bool recursive);

    // 调度操作系统在下次重启的时候删除文件或目录.
    // 注意:
    // 1) 待删除的文件/目录应该在临时目录.
    // 2) 待删除的目录必须为空.
    bool DeleteAfterReboot(const FilePath& path);

    // 返回文件路径的信息.
    bool GetFileInfo(const FilePath& file_path, base::PlatformFileInfo* info);

    // 封装fopen调用. 成功返回非空的FILE*.
    FILE* OpenFile(const FilePath& filename, const char* mode);

    // 关闭OpenFile打开的文件. 成功返回true.
    bool CloseFile(FILE* file);

    // 返回系统提供的临时目录.
    bool GetTempDir(FilePath* path);

    // 创建临时文件. |path|是全路径, 创建文件成功函数返回true. 函数返回时
    // 文件为空, 所有句柄都会关闭.
    bool CreateTemporaryFile(FilePath* path);

    // 类似CreateTemporaryFile, 文件在|dir|目录中创建.
    bool CreateTemporaryFileInDir(const FilePath& dir, FilePath* temp_file);

    // 创建并打开临时文件. 文件打开权限为读/写. |path|为全路径.
    // 返回打开文件的句柄, 发生错误返回NULL.
    FILE* CreateAndOpenTemporaryFile(FilePath* path);
    // 类似CreateAndOpenTemporaryFile, 文件在|dir|目录中创建.
    FILE* CreateAndOpenTemporaryFileInDir(const FilePath& dir, FilePath* path);

    // 读取|path|文件内容到|contents|, 成功返回true. |contents|可以为空, 此时
    // 函数用于判断磁盘文件是否存在.
    bool ReadFileToString(const FilePath& path, std::string* contents);

    // 从文件读取指定的字节内容到缓冲区. 返回实际读取到的字节数, 错误返回-1.
    int ReadFile(const FilePath& filename, char* data, int size);

    // 写入缓冲区内容到文件, 覆盖之前的数据. 返回实际写入的字节数, 错误返回-1.
    int WriteFile(const FilePath& filename, const char* data, int size);

    // 修改文件名|from_path|为|to_path|. 两个路径必须在相同的卷上, 否则函数会失败.
    // 目的文件不存在时会被创建. 处理临时文件时优先选择这个函数而不是Move. Windows
    // 平台上目标文件的属性会保留. 成功返回true.
    bool ReplaceFile(const FilePath& from_path, const FilePath& to_path);

    // 如果给定路径存在本地文件系统则返回true, 否则返回false.
    bool PathExists(const FilePath& path);

    // Returns true if the given path exists and is a directory, false otherwise.
    bool DirectoryExists(const FilePath& path);

    // 获取进程当前工作目录.
    bool GetCurrentDirectory(FilePath* path);

    // 设置进程当前工作目录.
    bool SetCurrentDirectory(const FilePath& path);

    // Creates a directory, as well as creating any parent directories, if they
    // don't exist. Returns 'true' on successful creation, or if the directory
    // already exists.  The directory is only readable by the current user.
    bool CreateDirectory(const FilePath& full_path);

    // Returns true if the given path's base name is ".".
    bool IsDot(const FilePath& path);

    // Returns true if the given path's base name is "..".
    bool IsDotDot(const FilePath& path);

    // 枚举路径下所有文件, 不保证次序.
    // 遍历操作是阻塞方式, 不要在主线程中使用.
    class FileEnumerator
    {
    public:
        typedef WIN32_FIND_DATA FindInfo;

        enum FILE_TYPE
        {
            FILES                 = 1 << 0,
            DIRECTORIES           = 1 << 1,
            INCLUDE_DOT_DOT       = 1 << 2,
        };

        // |root_path|是遍历的起始目录, 可能不以反斜线结尾.
        //
        // 如果|recursive|是true, 会递归遍历子目录. 采用广度优先遍历方式, 所以
        // 当前目录的文件先于子目录的文件返回.
        //
        // |file_type|指定是匹配文件或是目录或者两者都匹配.
        //
        // |pattern|是可选的文件匹配模式, 实现shell的部分特性, 比如"*.txt"或者
        // "Foo???.doc". 但是某些匹配模式不是跨平台的, 因为底层调用的是OS相关
        // 功能. 一般来说, Windows的匹配特性要少于其它平台, 优先测试. 如果没
        // 指定, 匹配所有文件.
        // 注意: 匹配模式仅限定root_path目录有效, 递归的子目录下不起作用.
        FileEnumerator(const FilePath& root_path,
            bool recursive,
            FileEnumerator::FILE_TYPE file_type);
        FileEnumerator(const FilePath& root_path,
            bool recursive,
            FileEnumerator::FILE_TYPE file_type,
            const FilePath::StringType& pattern);
        ~FileEnumerator();

        // 如果没有下一个路径返回空字符串.
        FilePath Next();

        // 写文件信息到|info|.
        void GetFindInfo(FindInfo* info);

        // 检查FindInfo是不是目录.
        static bool IsDirectory(const FindInfo& info);

        static FilePath GetFilename(const FindInfo& find_info);

    private:
        // 如果枚举可以跳过给定路径返回true.
        bool ShouldSkip(const FilePath& path);

        // 当find_data_合法时为true.
        bool has_find_data_;
        WIN32_FIND_DATA find_data_;
        HANDLE find_handle_;

        FilePath root_path_;
        bool recursive_;
        FILE_TYPE file_type_;
        std::wstring pattern_; // 不进行匹配时字符串为空.

        // 记录广度优先查找中还需要遍历的子目录.
        std::stack<FilePath> pending_paths_;

        DISALLOW_COPY_AND_ASSIGN(FileEnumerator);
    };


    class MemoryMappedFile
    {
    public:
        // 缺省构造函数, 所有成员都设置为非法/空值.
        MemoryMappedFile();
        ~MemoryMappedFile();

        // 打开一个存在的文件并映射到内存中. 访问权限制为只读. 如果对象已经指向一个
        // 合法的内存映射文件, 调用会失败并返回false. 如果无法打开文件、文件不存在
        // 或者内存映射失败, 函数返回false. 以后可能会允许指定访问权限.
        bool Initialize(const FilePath& file_name);
        // 和上面一样, 只是文件必须是已打开的. MemoryMappedFile会接管|file|的所有权,
        // 用完之后会关闭.
        bool Initialize(PlatformFile file);

        const uint8* data() const { return data_; }
        size_t length() const { return length_; }

        // file_是指向一个打开的内存映射文件的合法句柄吗?
        bool IsValid();

    private:
        // 打开指定文件, 传递给MapFileToMemoryInternal().
        bool MapFileToMemory(const FilePath& file_name);

        // 映射文件到内存, 设置data_为内存地址. 如果成功返回true, 任何失败情形都会
        // 返回false. Initialize()的辅助函数.
        bool MapFileToMemoryInternal();

        // MapFileToMemoryInternal调用该函数, 可以传递映射段的标志位.
        bool MapFileToMemoryInternalEx(int flags);

        // 关闭所有打开的句柄. 函数以后可能会变成公共的.
        void CloseHandles();

        base::PlatformFile file_;
        HANDLE file_mapping_;
        uint8* data_;
        size_t length_;

        DISALLOW_COPY_AND_ASSIGN(MemoryMappedFile);
    };

} //namespace base

#endif //__base_file_util_h__