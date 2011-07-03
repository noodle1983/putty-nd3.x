
#include "stringprintf.h"

#include <vector>

#include "string_util.h"

namespace base
{

    // vsnprintf和vswprintf的封装. buf_size是缓冲区的大小.
    // 返回格式化后字符串的长度不包含结尾的NUL.
    // 如果缓冲区不足以完成字符串的格式化, 返回完全格式化后的字符数(Windows).
    inline int vsnprintfT(char* buffer, size_t buf_size,
        const char* format, va_list argptr)
    {
        return base::vsnprintf(buffer, buf_size, format, argptr);
    }

    inline int vsnprintfT(wchar_t* buffer, size_t buf_size,
        const wchar_t* format, va_list argptr)
    {
        return base::vswprintf(buffer, buf_size, format, argptr);
    }

    // StringPrintF/StringAppendF的底层实现. 函数不负责重置va_list, 由调用者负责.
    template<typename StringType>
    static void StringAppendVT(StringType* dst,
        const typename StringType::value_type* format, va_list ap)
    {
        // 首先尝试用固定大小的小缓冲区.
        typename StringType::value_type stack_buf[1024];

        va_list ap_copy;
        GG_VA_COPY(ap_copy, ap);

        int result = vsnprintfT(stack_buf, arraysize(stack_buf), format, ap_copy);
        va_end(ap_copy);

        if(result>=0 && result<static_cast<int>(arraysize(stack_buf)))
        {
            // 缓冲区合适.
            dst->append(stack_buf, result);
            return;
        }

        // 持续增加缓冲区大小直到合适.
        int mem_length = arraysize(stack_buf);
        while(true)
        {
            if(result < 0)
            {
                // 如果发生错误而不是溢出, 不再继续执行.
                DLOG(WARNING) << "Unable to printf the requested string due to error.";
                return;
            }
            else
            {
                // 需要"result+1"个字符空间.
                mem_length = result + 1;
            }

            if(mem_length > 32*1024*1024)
            {
                // 已经足够大了, 不再继续尝试. 避免vsnprintfT在大块分配时返回-1而不是
                // 设置溢出.
                DLOG(WARNING) << "Unable to printf the requested string due to size.";
                return;
            }

            std::vector<typename StringType::value_type> mem_buf(mem_length);

            // 注意: va_list只能用一次. 循环中每次都产生一份新的拷贝而不使用原始的.
            GG_VA_COPY(ap_copy, ap);
            result = vsnprintfT(&mem_buf[0], mem_length, format, ap_copy);
            va_end(ap_copy);

            if((result>=0) && (result<mem_length))
            {
                // 缓冲区合适.
                dst->append(&mem_buf[0], result);
                return;
            }
        }
    }

    std::string StringPrintf(const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        std::string result;
        StringAppendV(&result, format, ap);
        va_end(ap);
        return result;
    }

    std::wstring StringPrintf(const wchar_t* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        std::wstring result;
        StringAppendV(&result, format, ap);
        va_end(ap);
        return result;
    }

    std::string StringPrintV(const char* format, va_list ap)
    {
        std::string result;
        StringAppendV(&result, format, ap);
        return result;
    }

    const std::string& SStringPrintf(std::string* dst, const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        dst->clear();
        StringAppendV(dst, format, ap);
        va_end(ap);
        return *dst;
    }

    const std::wstring& SStringPrintf(std::wstring* dst,
        const wchar_t* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        dst->clear();
        StringAppendV(dst, format, ap);
        va_end(ap);
        return *dst;
    }

    void StringAppendF(std::string* dst, const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        StringAppendV(dst, format, ap);
        va_end(ap);
    }

    void StringAppendF(std::wstring* dst, const wchar_t* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        StringAppendV(dst, format, ap);
        va_end(ap);
    }

    void StringAppendV(std::string* dst, const char* format, va_list ap)
    {
        StringAppendVT(dst, format, ap);
    }

    void StringAppendV(std::wstring* dst, const wchar_t* format, va_list ap)
    {
        StringAppendVT(dst, format, ap);
    }

} //namespace base