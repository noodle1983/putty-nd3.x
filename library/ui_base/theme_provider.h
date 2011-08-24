
#ifndef __ui_base_theme_provider_h__
#define __ui_base_theme_provider_h__

#pragma once

#include "base/basic_types.h"

#include "SkColor.h"

class RefCountedMemory;

class SkBitmap;

class Profile;

namespace ui
{

    ////////////////////////////////////////////////////////////////////////////////
    //
    // ThemeProvider
    //
    //   ThemeProvider is an abstract class that defines the API that should be
    //   implemented to provide bitmaps and color information for a given theme.
    //
    ////////////////////////////////////////////////////////////////////////////////

    class ThemeProvider
    {
    public:
        virtual ~ThemeProvider();

        // Initialize the provider with the passed in profile.
        virtual void Init(Profile* profile) = 0;

        // Get the bitmap specified by |id|. An implementation of ThemeProvider should
        // have its own source of ids (e.g. an enum, or external resource bundle).
        virtual SkBitmap* GetBitmapNamed(int id) const = 0;

        // Get the color specified by |id|.
        virtual SkColor GetColor(int id) const = 0;

        // Get the property (e.g. an alignment expressed in an enum, or a width or
        // height) specified by |id|.
        virtual bool GetDisplayProperty(int id, int* result) const = 0;

        // Whether we should use the native system frame (typically Aero glass) or
        // a custom frame.
        virtual bool ShouldUseNativeFrame() const = 0;

        // Whether or not we have a certain image. Used for when the default theme
        // doesn't provide a certain image, but custom themes might (badges, etc).
        virtual bool HasCustomImage(int id) const = 0;

        // Reads the image data from the theme file into the specified vector. Only
        // valid for un-themed resources and the themed IDR_THEME_NTP_* in most
        // implementations of ThemeProvider. Returns NULL on error.
        virtual RefCountedMemory* GetRawData(int id) const = 0;
    };

} //namespace ui

#endif //__ui_base_theme_provider_h__