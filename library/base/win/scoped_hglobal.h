
#ifndef __base_scoped_hglobal_h__
#define __base_scoped_hglobal_h__

#pragma once

#include <windows.h>

#include "base/basic_types.h"

namespace base
{
    namespace win
    {

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

    } //namespace win
} //namespace base

#endif //__base_scoped_hglobal_h__