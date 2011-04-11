
#ifndef __base_win_scoped_handle_h__
#define __base_win_scoped_handle_h__

#pragma once

#include <windows.h>

#include "../logging.h"

namespace base
{

    // 更好的管理句柄, 以免忘记关闭.
    // 类的接口和ScopedStdioHandle一样, 多了一个IsValid()方法, 因为Windows
    // 上的非法句柄可以是NULL或者INVALID_HANDLE_VALUE(-1).
    //
    // 示例:
    //     ScopedHandle hfile(CreateFile(...));
    //     if(!hfile.Get())
    //       ...process error
    //     ReadFile(hfile.Get(), ...);
    //
    // 转移句柄的所有权:
    //     secret_handle_ = hfile.Take();
    //
    // 显式的关闭句柄:
    //     hfile.Close();
    class ScopedHandle
    {
    public:
        ScopedHandle() : handle_(NULL) {}

        explicit ScopedHandle(HANDLE h) : handle_(NULL)
        {
            Set(h);
        }

        ~ScopedHandle()
        {
            Close();
        }

        // 该方法可以代替句柄和INVALID_HANDLE_VALUE的比较, 因为这里用NULL
        // 表示错误.
        bool IsValid() const
        {
            return handle_ != NULL;
        }

        void Set(HANDLE new_handle)
        {
            Close();

            // Windows的非法句柄表示方法是不一致的, 这里统一使用NULL.
            if(new_handle != INVALID_HANDLE_VALUE)
            {
                handle_ = new_handle;
            }
        }

        HANDLE Get()
        {
            return handle_;
        }

        operator HANDLE() { return handle_; }

        HANDLE Take()
        {
            // 转交所有权.
            HANDLE h = handle_;
            handle_ = NULL;
            return h;
        }

        void Close()
        {
            if(handle_)
            {
                if(!CloseHandle(handle_))
                {
                    NOTREACHED();
                }
                handle_ = NULL;
            }
        }

    private:
        HANDLE handle_;
        DISALLOW_COPY_AND_ASSIGN(ScopedHandle);
    };

    template<class T>
    class ScopedGDIObject
    {
    public:
        ScopedGDIObject() : object_(NULL) {}
        explicit ScopedGDIObject(T object) : object_(object) {}

        ~ScopedGDIObject()
        {
            Close();
        }

        T Get()
        {
            return object_;
        }

        void Set(T object)
        {
            if(object_ && object!=object_)
            {
                Close();
            }
            object_ = object;
        }

        ScopedGDIObject& operator=(T object)
        {
            Set(object);
            return *this;
        }

        T release()
        {
            T object = object_;
            object_ = NULL;
            return object;
        }

        operator T() { return object_; }

    private:
        void Close()
        {
            if(object_)
            {
                DeleteObject(object_);
            }
        }

        T object_;
        DISALLOW_COPY_AND_ASSIGN(ScopedGDIObject);
    };

    // 特例化HICON, 因为销毁HICON需要调用DestroyIcon()而不是DeleteObject().
    template<>
    void ScopedGDIObject<HICON>::Close()
    {
        if(object_)
        {
            DestroyIcon(object_);
        }
    }

    // 定义常用的类型.
    typedef ScopedGDIObject<HBITMAP> ScopedBitmap;
    typedef ScopedGDIObject<HRGN> ScopedRegion;
    typedef ScopedGDIObject<HFONT> ScopedHFONT;
    typedef ScopedGDIObject<HICON> ScopedHICON;

    // 只能用于从CreateCompatibleDC返回的HDCs. 从GetDC返回的HDC, 应该
    // 使用ReleaseDC.
    class ScopedHDC
    {
    public:
        ScopedHDC() : hdc_(NULL) {}
        explicit ScopedHDC(HDC h) : hdc_(h) {}

        ~ScopedHDC()
        {
            Close();
        }

        HDC Get()
        {
            return hdc_;
        }

        void Set(HDC h)
        {
            Close();
            hdc_ = h;
        }

        operator HDC() { return hdc_; }

    private:
        void Close()
        {
#ifdef NOGDI
            assert(false);
#else
            if(hdc_)
            {
                DeleteDC(hdc_);
            }
#endif //NOGDI
        }

        HDC hdc_;
        DISALLOW_COPY_AND_ASSIGN(ScopedHDC);
    };

    template<class T>
    class ScopedHGlobal
    {
    public:
        explicit ScopedHGlobal(HGLOBAL glob) : glob_(glob)
        {
            data_ = static_cast<T*>(GlobalLock(glob_));
        }
        ~ScopedHGlobal()
        {
            GlobalUnlock(glob_);
        }

        T* get() { return data_; }

        size_t Size() const { return GlobalSize(glob_); }

        T* operator->() const 
        {
            assert(data_ != 0);
            return data_;
        }

        T* release()
        {
            T* data = data_;
            data_ = NULL;
            return data;
        }

    private:
        HGLOBAL glob_;
        T* data_;

        DISALLOW_COPY_AND_ASSIGN(ScopedHGlobal);
    };

    class ScopedStdioHandle
    {
    public:
        ScopedStdioHandle() : handle_(NULL) {}

        explicit ScopedStdioHandle(FILE* handle)
            : handle_(handle) {}

        ~ScopedStdioHandle()
        {
            Close();
        }

        void Close()
        {
            if(handle_)
            {
                fclose(handle_);
                handle_ = NULL;
            }
        }

        FILE* get() const { return handle_; }

        FILE* Take()
        {
            FILE* temp = handle_;
            handle_ = NULL;
            return temp;
        }

        void Set(FILE* newhandle)
        {
            Close();
            handle_ = newhandle;
        }

    private:
        FILE* handle_;

        DISALLOW_COPY_AND_ASSIGN(ScopedStdioHandle);
    };

} //namespace base

#endif //__base_win_scoped_handle_h__