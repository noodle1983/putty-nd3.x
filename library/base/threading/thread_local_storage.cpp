
#include "thread_local_storage.h"

#include <windows.h>

#include "base/logging.h"

namespace base
{

    // 为了使TLS的销毁函数工作, 需要保存每个分配的TLS的销毁函数指针.
    // 这里只分配一个OS级别的TLS, 包含一组插槽供应用程序使用. 同时,
    // 分配一组销毁函数指针数组, 在线程终止的时候调用.

    // tls_key_是本地TLS. 里面存放的是一张表.
    long ThreadLocalStorage::tls_key_ = TLS_OUT_OF_INDEXES;

    // tls_max_是已分配的插槽上界. 内部跳过0, 避免和未分配的TLS插槽混淆.
    long ThreadLocalStorage::tls_max_ = 1;

    // 对于插槽的销毁函数指针数组. 如果一个插槽有销毁函数, 会存储函数
    // 指针到插槽索引位置.
    ThreadLocalStorage::TLSDestructorFunc
        ThreadLocalStorage::tls_destructors_[kThreadLocalStorageSize];

    void** ThreadLocalStorage::Initialize()
    {
        if(tls_key_ == TLS_OUT_OF_INDEXES)
        {
            long value = TlsAlloc();
            DCHECK(value != TLS_OUT_OF_INDEXES);

            // 测试并赋值tls_key原子操作. 如果key是TLS_OUT_OF_INDEXES, 设置其值.
            // 否则, 什么也不做, 因为另外一个线程替我们做了这件事.
            if(InterlockedCompareExchange(&tls_key_, value,
                TLS_OUT_OF_INDEXES) != TLS_OUT_OF_INDEXES)
            {
                // 另外一个线程抢先赋值了, 所以销毁自己的TLS索引,
                // 使用另外线程先分配的值.
                TlsFree(value);
            }
        }
        DCHECK(TlsGetValue(tls_key_) == NULL);

        // 创建数组存储数据.
        void** tls_data = new void*[kThreadLocalStorageSize];
        memset(tls_data, 0, sizeof(void*[kThreadLocalStorageSize]));
        TlsSetValue(tls_key_, tls_data);
        return tls_data;
    }

    ThreadLocalStorage::Slot::Slot(TLSDestructorFunc destructor)
        : initialized_(false)
    {
        Initialize(destructor);
    }

    bool ThreadLocalStorage::Slot::Initialize(TLSDestructorFunc destructor)
    {
        if(tls_key_==TLS_OUT_OF_INDEXES || !TlsGetValue(tls_key_))
        {
            ThreadLocalStorage::Initialize();
        }

        // 获取一个新的插槽.
        slot_ = InterlockedIncrement(&tls_max_) - 1;
        if(slot_ >= kThreadLocalStorageSize)
        {
            NOTREACHED();
            return false;
        }

        // 设置销毁函数指针.
        tls_destructors_[slot_] = destructor;
        initialized_ = true;
        return true;
    }

    void ThreadLocalStorage::Slot::Free()
    {
        // 这里不收回旧的TLS插槽索引, 只用清理析构函数指针.
        tls_destructors_[slot_] = NULL;
        initialized_ = false;
    }

    void* ThreadLocalStorage::Slot::Get() const
    {
        void** tls_data = static_cast<void**>(TlsGetValue(tls_key_));
        if(!tls_data)
        {
            tls_data = ThreadLocalStorage::Initialize();
        }
        DCHECK(slot_>=0 && slot_<kThreadLocalStorageSize);
        return tls_data[slot_];
    }

    void ThreadLocalStorage::Slot::Set(void* value)
    {
        void** tls_data = static_cast<void**>(TlsGetValue(tls_key_));
        if(!tls_data)
        {
            tls_data = ThreadLocalStorage::Initialize();
        }
        DCHECK(slot_>=0 && slot_<kThreadLocalStorageSize);
        tls_data[slot_] = value;
    }

    void ThreadLocalStorage::ThreadExit()
    {
        if(tls_key_ == TLS_OUT_OF_INDEXES)
        {
            return;
        }

        void** tls_data = static_cast<void**>(TlsGetValue(tls_key_));

        // 本线程有可能没有初始化TLS.
        if(!tls_data)
        {
            return;
        }

        for(int slot=0; slot<tls_max_; slot++)
        {
            if(tls_destructors_[slot] != NULL)
            {
                void* value = tls_data[slot];
                tls_destructors_[slot](value);
            }
        }

        delete[] tls_data;

        // 以免其它"onexit"处理函数...
        TlsSetValue(tls_key_, NULL);
    }

} //namespace base

// Thread Termination Callbacks.
// Windows doesn't support a per-thread destructor with its
// TLS primitives.  So, we build it manually by inserting a
// function to be called on each thread's exit.
// This magic is from http://www.codeproject.com/threads/tls.asp
// and it works for VC++ 7.0 and later.

// Force a reference to _tls_used to make the linker create the TLS directory
// if it's not already there.  (e.g. if __declspec(thread) is not used).
// Force a reference to p_thread_callback_base to prevent whole program
// optimization from discarding the variable.
#ifdef _WIN64
#pragma comment(linker, "/INCLUDE:_tls_used")
#pragma comment(linker, "/INCLUDE:p_thread_callback_base")
#else //_WIN64
#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:_p_thread_callback_base")
#endif //_WIN64

// Static callback function to call with each thread termination.
void NTAPI OnThreadExit(PVOID module, DWORD reason, PVOID reserved)
{
    // On XP SP0 & SP1, the DLL_PROCESS_ATTACH is never seen. It is sent on SP2+
    // and on W2K and W2K3. So don't assume it is sent.
    if(DLL_THREAD_DETACH==reason || DLL_PROCESS_DETACH==reason)
    {
        base::ThreadLocalStorage::ThreadExit();
    }
}

// .CRT$XLA to .CRT$XLZ is an array of PIMAGE_TLS_CALLBACK pointers that are
// called automatically by the OS loader code (not the CRT) when the module is
// loaded and on thread creation. They are NOT called if the module has been
// loaded by a LoadLibrary() call. It must have implicitly been loaded at
// process startup.
// By implicitly loaded, I mean that it is directly referenced by the main EXE
// or by one of its dependent DLLs. Delay-loaded DLL doesn't count as being
// implicitly loaded.
//
// See VC\crt\src\tlssup.c for reference.

// extern "C" suppresses C++ name mangling so we know the symbol name for the
// linker /INCLUDE:symbol pragma above.
extern "C"
{
    // The linker must not discard p_thread_callback_base.  (We force a reference
    // to this variable with a linker /INCLUDE:symbol pragma to ensure that.) If
    // this variable is discarded, the OnThreadExit function will never be called.
#ifdef _WIN64
    // .CRT section is merged with .rdata on x64 so it must be constant data.
#pragma const_seg(".CRT$XLB")
    // When defining a const variable, it must have external linkage to be sure the
    // linker doesn't discard it.
    extern const PIMAGE_TLS_CALLBACK p_thread_callback_base;
    const PIMAGE_TLS_CALLBACK p_thread_callback_base = OnThreadExit;

    // Reset the default section.
#pragma const_seg()
#else //_WIN64
#pragma data_seg(".CRT$XLB")
    PIMAGE_TLS_CALLBACK p_thread_callback_base = OnThreadExit;

    // Reset the default section.
#pragma data_seg()
#endif //_WIN64
} //extern "C"