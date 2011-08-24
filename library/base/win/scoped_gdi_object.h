
#ifndef __base_scoped_gdi_object_h__
#define __base_scoped_gdi_object_h__

#pragma once

#include <windows.h>

#include "base/basic_types.h"

namespace base
{
    namespace win
    {

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

    } //namespace win
} //namespace base

#endif //__base_scoped_gdi_object_h__