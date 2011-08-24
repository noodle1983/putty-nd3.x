
#ifndef __ui_gfx_favicon_size_h__
#define __ui_gfx_favicon_size_h__

#pragma once

// Size (along each axis) of the favicon.
const int kFaviconSize = 16;

// If the width or height is bigger than the favicon size, a new width/height
// is calculated and returned in width/height that maintains the aspect
// ratio of the supplied values.
static void calc_favicon_target_size(int* width, int* height);

// static
void calc_favicon_target_size(int* width, int* height)
{
    if(*width>kFaviconSize || *height>kFaviconSize)
    {
        // Too big, resize it maintaining the aspect ratio.
        float aspect_ratio = static_cast<float>(*width) /
            static_cast<float>(*height);
        *height = kFaviconSize;
        *width = static_cast<int>(aspect_ratio * *height);
        if(*width > kFaviconSize)
        {
            *width = kFaviconSize;
            *height = static_cast<int>(*width / aspect_ratio);
        }
    }
}

#endif //__ui_gfx_favicon_size_h__