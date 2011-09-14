
#ifndef __base_scoped_co_mem_h__
#define __base_scoped_co_mem_h__

#pragma once

#include <objbase.h>

#include "base/basictypes.h"

namespace base
{
    namespace win
    {

        // Simple scoped memory releaser class for COM allocated memory.
        // Example:
        //   chrome::common::ScopedCoMem<ITEMIDLIST> file_item;
        //   SHGetSomeInfo(&file_item, ...);
        //   ...
        //   return;  <-- memory released
        template<typename T>
        class ScopedCoMem
        {
        public:
            explicit ScopedCoMem() : mem_ptr_(NULL) {}

            ~ScopedCoMem()
            {
                if(mem_ptr_)
                {
                    CoTaskMemFree(mem_ptr_);
                }
            }

            T** operator&()
            {
                // NOLINT
                return &mem_ptr_;
            }

            operator T*()
            {
                return mem_ptr_;
            }

        private:
            T* mem_ptr_;

            DISALLOW_COPY_AND_ASSIGN(ScopedCoMem);
        };

    } //namespace win
} //namespace base

#endif //__base_scoped_co_mem_h__