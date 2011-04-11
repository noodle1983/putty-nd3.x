
#include "wrapped_window_proc.h"

#include "../atomicops.h"

namespace
{
    base::WinProcExceptionFilter s_exception_filter = NULL;
}

namespace base
{

    WinProcExceptionFilter SetWinProcExceptionFilter(
        WinProcExceptionFilter filter)
    {
        AtomicWord rv = NoBarrier_AtomicExchange(
            reinterpret_cast<AtomicWord*>(&s_exception_filter),
            reinterpret_cast<AtomicWord>(filter));
        return reinterpret_cast<WinProcExceptionFilter>(rv);
    }

    int CallExceptionFilter(EXCEPTION_POINTERS* info)
    {
        return s_exception_filter ? s_exception_filter(info) :
            EXCEPTION_CONTINUE_SEARCH;
    }

} //namespace base