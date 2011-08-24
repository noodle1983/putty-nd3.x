
#ifndef __ui_gfx_image_util_h__
#define __ui_gfx_image_util_h__

#pragma once

#include <vector>

namespace gfx
{

    class Image;

    // Creates an image from the given PNG-encoded input.  The caller owns the
    // returned Image.  If there was an error creating the image, returns NULL.
    Image* ImageFromPNGEncodedData(const unsigned char* input, size_t input_size);

    // Fills the |dst| vector with PNG-encoded bytes based on the given Image.
    // Returns true if the Image was encoded successfully.
    bool PNGEncodedDataFromImage(const Image& image,
        std::vector<unsigned char>* dst);

    // Fills the |dst| vector with JPEG-encoded bytes based on the given Image.
    // Returns true if the Image was encoded successfully.
    bool JPEGEncodedDataFromImage(const Image& image,
        std::vector<unsigned char>* dst);

} //namespace gfx

#endif //__ui_gfx_image_util_h__