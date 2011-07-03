
#include "base/logging.h"
#include "base/stringprintf.h"

void SkDebugf_FileLine(const char* file, int line, bool fatal,
                       const char* format, ...)
{
    va_list ap;
    va_start(ap, format);

    std::string msg;
    base::StringAppendV(&msg, format, ap);

    base::LogMessage(file, line,
        fatal?base::LOG_FATAL:base::LOG_INFO).stream() << msg;
}