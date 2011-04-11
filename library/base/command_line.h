
#ifndef __base_command_line_h__
#define __base_command_line_h__

#pragma once

#include <map>
#include <string>
#include <vector>

class FilePath;

namespace base
{

    // 构建解析命令行类.
    // 可选开关可以通过等号后面带一个数值添加, 比如"-switch=value".
    // 不以switch为前缀的参数保存为附加参数. 以"--"结尾的参数终止switch的解析,
    // 导致后面所有内容保存为一个附加参数.
    // 用一个只读的CommandLine单件表示当前进程启动的命令行, 必须在main()函数中
    // 初始化(或者其他平台等同函数).

    class CommandLine
    {
    public:
        // 命令行参数类型
        typedef std::wstring StringType;

        typedef std::vector<StringType> StringVector;
        // map类型: 解析出来的开关的key和values
        typedef std::map<std::string, StringType> SwitchMap;

        // 构建一个新的空命令行. |program|是将运行的程序名(argv[0]).
        enum NoProgram { NO_PROGRAM };
        explicit CommandLine(NoProgram no_program);

        // 通过argv[0]构造新的CommandLine对象.
        explicit CommandLine(const FilePath& program);

        ~CommandLine();

        // 初始化当前进程命令行CommandLine单件. 在Windows平台忽略掉它给的
        // 参数, 直接解析GetCommandLineW(). 因为CRT解析的命令行并不可靠,
        // 但我们还是需要调用CommandLineToArgvW处理命令行.
        static void Init(int argc, const char* const* argv);

        // 销毁当前进程的CommandLine单件. 当需要重置底层库到初始状态时调用(例如外部
        // 调用库需要结束并重新初始化时). 如果Init只被调用过一次, 例如在main()函数中,
        // 不必要强制调用Reset().
        static void Reset();

        // CommandLine单件表示当前进程的命令行. 注意: 返回值是可修改的,
        // 非线程安全, 所以修改时要确保正确性.
        static CommandLine* ForCurrentProcess();

        static CommandLine FromString(const std::wstring& command_line);

        // 返回初始的命令行字符串. 警告! 尽量不要使用, 因为引号的行为是不明确的.
        StringType command_line_string() const;

        // 返回命令行中的程序名(第一个字符串).
        FilePath GetProgram() const;

        // 如果命令行包含指定开关则返回true.(开关名大小写无关)
        bool HasSwitch(const std::string& switch_string) const;

        // 返回指定开关的值. 如果开关无值或者不存在, 返回空字符串.
        std::string GetSwitchValueASCII(const std::string& switch_string) const;
        FilePath GetSwitchValuePath(const std::string& switch_string) const;
        StringType GetSwitchValueNative(const std::string& switch_string) const;

        // 得到进程的所有开关数.
        size_t GetSwitchCount() const { return switches_.size(); }

        // 获取所有的开关
        const SwitchMap& GetSwitches() const { return switches_; }

        // 为命令行添加开关[值可选].
        // 警告! 在开关结束符"--"后添加是无效的.
        void AppendSwitch(const std::string& switch_string);
        void AppendSwitchPath(const std::string& switch_string,
            const FilePath& path);
        void AppendSwitchNative(const std::string& switch_string,
            const StringType& value);
        void AppendSwitchASCII(const std::string& switch_string,
            const std::string& value);
        void AppendSwitches(const CommandLine& other);

        // 从另一个命令行中拷贝指定的开关(包括值, 如果存在). 一般用于启动子进程.
        void CopySwitchesFrom(const CommandLine& source,
            const char* const switches[], size_t count);

        // 获取其它命令行参数
        const StringVector& args() const { return args_; }

        // 添加参数.
        // 注意引号: 必要时用引号把参数括起来以便被正确翻译为一个参数.
        // AppendArg主要接受ASCII的; 非ASCII输入会被认为是UTF-8编码格式.
        void AppendArg(const std::string& value);
        void AppendArgPath(const FilePath& value);
        void AppendArgNative(const StringType& value);
        void AppendArgs(const CommandLine& other);

        // 添加另一个命令行的所有参数. 如果|include_program|是true, |other|
        // 的程序名也会被添加进来.
        void AppendArguments(const CommandLine& other, bool include_program);

        // 在当前命令中插入命令, 常用语调试器, 像"valgrind" 或者 "gdb --args".
        void PrependWrapper(const StringType& wrapper);

        // 通过解析命令行字符串进行初始化, 假设程序名被是第一个字符串.
        void ParseFromString(const std::wstring& command_line);

    private:
        CommandLine();

        // CommandLine单件表示当前进程的命令行.
        static CommandLine* current_process_commandline_;

        // 用引号括起来, 以空格分隔的命令行字符串
        StringType command_line_string_;
        // 程序名
        StringType program_;

        // 解析出的开关值对.
        SwitchMap switches_;

        // 非开关的命令行参数.
        StringVector args_;

        // 允许拷贝构造函数, 因为经常会拷贝当前进程的命令行并添加标志位. 例如:
        //     CommandLine cl(*CommandLine::ForCurrentProcess());
        //     cl.AppendSwitch(...);
    };

} //namespace base

#endif //__base_command_line_h__