
#include "wrapped_window_proc.h"

#include "base/atomicops.h"

namespace
{
    base::win::WinProcExceptionFilter s_exception_filter = NULL;
}

namespace base
{
    namespace win
    {

        WinProcExceptionFilter SetWinProcExceptionFilter(
            WinProcExceptionFilter filter)
        {
            base::subtle::AtomicWord rv = base::subtle::NoBarrier_AtomicExchange(
                reinterpret_cast<base::subtle::AtomicWord*>(&s_exception_filter),
                reinterpret_cast<base::subtle::AtomicWord>(filter));
            return reinterpret_cast<WinProcExceptionFilter>(rv);
        }

        int CallExceptionFilter(EXCEPTION_POINTERS* info)
        {
            return s_exception_filter ? s_exception_filter(info) :
                EXCEPTION_CONTINUE_SEARCH;
        }

    } //namespace win
} //namespace base