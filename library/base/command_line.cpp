
#include "command_line.h"

#include <windows.h>

#include <algorithm>

#include "file_path.h"
#include "logging.h"
#include "string_split.h"
#include "string_util.h"
#include "utf_string_conversions.h"

namespace
{

    // 因为采用lazy匹配, 所以前缀长的(L"--")必须放在短的(L"-")前面.
    const CommandLine::CharType kSwitchTerminator[] = FILE_PATH_LITERAL("--");
    const CommandLine::CharType kSwitchValueSeparator[] = FILE_PATH_LITERAL("=");
    const CommandLine::CharType* const kSwitchPrefixes[] = { L"--", L"-", L"/" };

    size_t GetSwitchPrefixLength(const CommandLine::StringType& string)
    {
        for(size_t i=0; i<arraysize(kSwitchPrefixes); ++i)
        {
            CommandLine::StringType prefix(kSwitchPrefixes[i]);
            if(string.find(prefix) == 0)
            {
                return prefix.length();
            }
        }
        return 0;
    }

    // 必要时用引号括起来, 这样CommandLineToArgvW()就会把它处理成一个参数.
    std::wstring QuoteForCommandLineToArgvW(const std::wstring& arg)
    {
        // 遵守CommandLineToArgvW的引号规则.
        // http://msdn.microsoft.com/en-us/library/17w5ykft.aspx
        if(arg.find_first_of(L" \\\"") == std::wstring::npos)
        {
            // 没有添加引号必要.
            return arg;
        }

        std::wstring out;
        out.push_back(L'"');
        for(size_t i=0; i<arg.size(); ++i)
        {
            if(arg[i] == '\\')
            {
                // 查找反斜杠总数
                size_t start = i, end = start + 1;
                for(; end<arg.size()&&arg[end]=='\\'; ++end)
                {
                    /* empty */;
                }
                size_t backslash_count = end - start;

                // 后面跟着"的反斜杠会被转义. 因为会在字符串结尾添加一个",
                // 所以在"号或者结尾处进行转义反操作.
                if(end==arg.size() || arg[end]=='"')
                {
                    // 需要输出2倍数量的反斜杠.
                    backslash_count *= 2;
                }
                for(size_t j=0; j<backslash_count; ++j)
                {
                    out.push_back('\\');
                }

                // 更新i位置(注意循环中的++i)
                i = end - 1;
            }
            else if(arg[i] == '"')
            {
                out.push_back('\\');
                out.push_back('"');
            }
            else
            {
                out.push_back(arg[i]);
            }
        }
        out.push_back('"');

        return out;
    }

    // 返回true并填充|switch_string|和|switch_value|如果parameter_string
    // 表示一个开关.
    bool IsSwitch(const CommandLine::StringType& string,
        CommandLine::StringType* switch_string,
        CommandLine::StringType* switch_value)
    {
        switch_string->clear();
        switch_value->clear();
        if(GetSwitchPrefixLength(string) == 0)
        {
            return false;
        }

        const size_t equals_position = string.find(kSwitchValueSeparator);
        *switch_string = string.substr(0, equals_position);
        if(equals_position != CommandLine::StringType::npos)
        {
            *switch_value = string.substr(equals_position+1);
        }
        return true;
    }

    // Append switches and arguments, keeping switches before arguments.
    void AppendSwitchesAndArguments(CommandLine& command_line,
        const CommandLine::StringVector& argv)
    {
        bool parse_switches = true;
        for(size_t i=1; i<argv.size(); ++i)
        {
            CommandLine::StringType arg = argv[i];
            TrimWhitespace(arg, TRIM_ALL, &arg);

            CommandLine::StringType switch_string;
            CommandLine::StringType switch_value;
            parse_switches &= (arg != kSwitchTerminator);
            if(parse_switches && IsSwitch(arg, &switch_string, &switch_value))
            {
                command_line.AppendSwitchNative(WideToASCII(switch_string),
                    switch_value);
            }
            else
            {
                command_line.AppendArgNative(arg);
            }
        }
    }

    // Lowercase switches for backwards compatiblity *on Windows*.
    std::string LowerASCIIOnWindows(const std::string& string)
    {
        return StringToLowerASCII(string);
    }

}


CommandLine* CommandLine::current_process_commandline_ = NULL;

CommandLine::CommandLine(NoProgram no_program)
: argv_(1), begin_args_(1) {}

CommandLine::CommandLine(const FilePath& program)
: argv_(1), begin_args_(1)
{
    SetProgram(program);
}

CommandLine::CommandLine(int argc, const CommandLine::CharType* const* argv)
: argv_(1), begin_args_(1)
{
    InitFromArgv(argc, argv);
}

CommandLine::CommandLine(const StringVector& argv)
: argv_(1), begin_args_(1)
{
    InitFromArgv(argv);
}

CommandLine::~CommandLine() {}

void CommandLine::Init(int argc, const char* const* argv)
{
    if(current_process_commandline_)
    {
        // If this is intentional, Reset() must be called first. If we are using
        // the shared build mode, we have to share a single object across multiple
        // shared libraries.
        return;
    }

    current_process_commandline_ = new CommandLine(NO_PROGRAM);
    current_process_commandline_->ParseFromString(::GetCommandLineW());
}

void CommandLine::Reset()
{
    DCHECK(current_process_commandline_ != NULL);
    delete current_process_commandline_;
    current_process_commandline_ = NULL;
}

CommandLine* CommandLine::ForCurrentProcess()
{
    DCHECK(current_process_commandline_);
    return current_process_commandline_;
}

CommandLine CommandLine::FromString(const std::wstring& command_line)
{
    CommandLine cmd(NO_PROGRAM);
    cmd.ParseFromString(command_line);
    return cmd;
}

void CommandLine::InitFromArgv(int argc,
                               const CommandLine::CharType* const* argv)
{
    StringVector new_argv;
    for(int i=0; i<argc; ++i)
    {
        new_argv.push_back(argv[i]);
    }
    InitFromArgv(new_argv);
}

void CommandLine::InitFromArgv(const StringVector& argv)
{
    argv_ = StringVector(1);
    begin_args_ = 1;
    SetProgram(argv.empty() ? FilePath() : FilePath(argv[0]));
    AppendSwitchesAndArguments(*this, argv);
}

CommandLine::StringType CommandLine::GetCommandLineString() const
{
    StringType string(argv_[0]);
    string = QuoteForCommandLineToArgvW(string);
    // Append switches and arguments.
    bool parse_switches = true;
    for(size_t i=1; i<argv_.size(); ++i)
    {
        CommandLine::StringType arg = argv_[i];
        CommandLine::StringType switch_string;
        CommandLine::StringType switch_value;
        parse_switches &= arg != kSwitchTerminator;
        string.append(StringType(FILE_PATH_LITERAL(" ")));
        if (parse_switches && IsSwitch(arg, &switch_string, &switch_value))
        {
            string.append(switch_string);
            if(!switch_value.empty())
            {
                switch_value = QuoteForCommandLineToArgvW(switch_value);
                string.append(kSwitchValueSeparator + switch_value);
            }
        }
        else
        {
            arg = QuoteForCommandLineToArgvW(arg);
            string.append(arg);
        }
    }
    return string;
}

FilePath CommandLine::GetProgram() const
{
    return FilePath(argv_[0]);
}

void CommandLine::SetProgram(const FilePath& program)
{
    TrimWhitespace(program.value(), TRIM_ALL, &argv_[0]);
}

bool CommandLine::HasSwitch(const std::string& switch_string) const
{
    return switches_.find(LowerASCIIOnWindows(switch_string)) !=
        switches_.end();
}

std::string CommandLine::GetSwitchValueASCII(
    const std::string& switch_string) const
{
    StringType value = GetSwitchValueNative(switch_string);
    if(!IsStringASCII(value))
    {
        LOG(WARNING) << "Value of --" << switch_string << " must be ASCII.";
        return "";
    }
    return WideToASCII(value);
}

FilePath CommandLine::GetSwitchValuePath(
    const std::string& switch_string) const
{
    return FilePath(GetSwitchValueNative(switch_string));
}

CommandLine::StringType CommandLine::GetSwitchValueNative(
    const std::string& switch_string) const
{
    SwitchMap::const_iterator result = switches_.end();
    result = switches_.find(LowerASCIIOnWindows(switch_string));
    return result==switches_.end() ? StringType() : result->second;
}

void CommandLine::AppendSwitch(const std::string& switch_string)
{
    AppendSwitchNative(switch_string, StringType());
}

void CommandLine::AppendSwitchPath(const std::string& switch_string,
                                   const FilePath& path)
{
    AppendSwitchNative(switch_string, path.value());
}

void CommandLine::AppendSwitchNative(const std::string& switch_string,
                                     const CommandLine::StringType& value)
{
    std::string switch_key(LowerASCIIOnWindows(switch_string));
    StringType combined_switch_string(ASCIIToWide(switch_key));
    size_t prefix_length = GetSwitchPrefixLength(combined_switch_string);
    switches_[switch_key.substr(prefix_length)] = value;
    // Preserve existing switch prefixes in |argv_|; only append one if necessary.
    if(prefix_length == 0)
    {
        combined_switch_string = kSwitchPrefixes[0] + combined_switch_string;
    }
    if(!value.empty())
    {
        combined_switch_string += kSwitchValueSeparator + value;
    }
    // Append the switch and update the switches/arguments divider |begin_args_|.
    argv_.insert(argv_.begin()+begin_args_++, combined_switch_string);
}

void CommandLine::AppendSwitchASCII(const std::string& switch_string,
                                    const std::string& value_string)
{
    AppendSwitchNative(switch_string, ASCIIToWide(value_string));
}

void CommandLine::CopySwitchesFrom(const CommandLine& source,
                                   const char* const switches[], size_t count)
{
    for(size_t i=0; i<count; ++i)
    {
        if(source.HasSwitch(switches[i]))
        {
            AppendSwitchNative(switches[i],
                source.GetSwitchValueNative(switches[i]));
        }
    }
}

CommandLine::StringVector CommandLine::GetArgs() const
{
    // Gather all arguments after the last switch (may include kSwitchTerminator).
    StringVector args(argv_.begin()+begin_args_, argv_.end());
    // Erase only the first kSwitchTerminator (maybe "--" is a legitimate page?)
    StringVector::iterator switch_terminator =
        std::find(args.begin(), args.end(), kSwitchTerminator);
    if(switch_terminator != args.end())
    {
        args.erase(switch_terminator);
    }
    return args;
}

void CommandLine::AppendArg(const std::string& value)
{
    DCHECK(IsStringUTF8(value));
    AppendArgNative(UTF8ToWide(value));
}

void CommandLine::AppendArgPath(const FilePath& path)
{
    AppendArgNative(path.value());
}

void CommandLine::AppendArgNative(const StringType& value)
{
    argv_.push_back(value);
}

void CommandLine::AppendArguments(const CommandLine& other,
                                  bool include_program)
{
    if(include_program)
    {
        SetProgram(other.GetProgram());
    }
    AppendSwitchesAndArguments(*this, other.argv());
}

void CommandLine::PrependWrapper(const CommandLine::StringType& wrapper)
{
    if(wrapper.empty())
    {
        return;
    }
    // wrapper可能包含参数(像"gdb --args"). 所以我们不做任何额外的处理, 仅以空格
    // 进行切分.
    StringVector wrapper_argv;
    base::SplitString(wrapper, FILE_PATH_LITERAL(' '), &wrapper_argv);
    // Prepend the wrapper and update the switches/arguments |begin_args_|.
    argv_.insert(argv_.begin(), wrapper_argv.begin(), wrapper_argv.end());
    begin_args_ += wrapper_argv.size();
}

void CommandLine::ParseFromString(const std::wstring& command_line)
{
    std::wstring command_line_string;
    TrimWhitespace(command_line, TRIM_ALL, &command_line_string);
    if(command_line_string.empty())
    {
        return;
    }

    int num_args = 0;
    wchar_t** args = NULL;
    args = ::CommandLineToArgvW(command_line_string.c_str(), &num_args);

    PLOG_IF(FATAL, !args) << "CommandLineToArgvW failed on command line: " <<
        command_line;
    InitFromArgv(num_args, args);
    LocalFree(args);
}