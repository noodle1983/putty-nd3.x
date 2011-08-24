
#ifndef __base_scoped_handle_h__
#define __base_scoped_handle_h__

#pragma once

#include <windows.h>

#include "base/logging.h"

namespace base
{
    namespace win
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

    } //namespace win
} //namespace base

#endif //__base_scoped_handle_h__